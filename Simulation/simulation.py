import numpy as np

import matplotlib.pyplot as plt

from matplotlib.animation import FuncAnimation

import matplotlib.patches as patches



# --- PARAMETRY FIZYCZNE ROBOTA ---

m_robot = 0.101             # masa robota [kg]
m_wheel = 0.0125
r_wheel = 0.015             # promien kola [m]
L = 0.1                     # odleglosc miedzy kolami [m]

T = 0.785
d = 0.036
g = 9.81
J_robot = (T**2*m_robot*g*d)/(4*np.pi*np.pi)  # przybl. moment bezwladnosci robota 
J_wheel = (m_wheel * r_wheel**2) / 2  # moment bezwladnosci kola


# --- PARAMETRY NAPEDU I TARCIA ---

R_motor = 5.0               # opor wewnetrzny silnika [Ohm]
Kt = 0.02                   # stala momentu [Nm/A]
Ke = 0.02                   # stala SEM [V/(rad/s)]
V_bat = 6.0                 # napiecie zasilania [V]
mu_friction = 0.02          # wspolczynnik tarcia statycznego
mu_dynamic = 0.01           # wspolczynnik tarcia dynamicznego
noise_level = 0.005         # poziom szumu czujnikow



# --- CZUJNIKI ---

theta_sensors = np.deg2rad([-15, -11, -6.8, -2.2, 2.2, 6.8, 11, 15])
sensor_distances = 0.175  # odleglosc czujnikow od srodka robota [m]

sensor_widths = np.ones(len(theta_sensors)) * 0.003  # szerokosc czujnikow [m]


# --- PARAMETRY KONTROLERA ---

Kp = 5.0                    # wzmocnienie proporcjonalne
Ki = 0.0                    # wzmocnienie calkujace
Kd = 0.0                    # wzmocnienie rozniczkujace
integral_error = 0.0        # zmienna do całkowania błędu
last_error = 0.0            # poprzedni błąd dla członu różniczkującego



# --- SYMULACJA ---

dt = 0.001
T = 10.0                    # Dłuższy czas symulacji dla skomplikowanego toru
N = int(T/dt)



# --- STANY ---

x, y = 0.0, 0.09            # start poza torem
theta = np.deg2rad(-45)     # początkowy kąt orientacji robota - start w kierunku "w dół"
v = 0.0                     # prędkość liniowa
omega = 0.0                 # prędkość kątowa
wL, wR = 0.0, 0.0           # prędkości kątowe kół


# --- CHARAKTERYSTYKA LINII ---

line_width = 0.019           # szerokosc linii [m]
line_color = 1            # intensywność koloru linii (0-1, gdzie 1 to czarny)

# --- DEFINICJA SKOMPLIKOWANEGO TORU ---

def complex_track(x, y, cx=0.5, cy=0.5, r=0.3):
    distance = np.sqrt((x - cx)**2 + (y - cy)**2)
    return abs(distance - r) < 0.002  # tu zostaje jako binarna maska

def line_intensity(x, y):
    # Stałe parametry toru
    cx, cy, r = 0.75, 0.75, 1
    distance = np.sqrt((x - cx)**2 + (y - cy)**2)
    delta = abs(distance - r)

    # Gaussian falloff (wokół toru)
    return line_color * np.exp(-(delta**2)/(2*(line_width/3)**2)) if delta < line_width else 0

# --- DETEKCJA LINII Z CZUJNIKOW ---
def detect_theta_error(x_robot, y_robot, theta_robot):

    # Obliczenie pozycji każdego czujnika
    sensor_pos_x = x_robot + sensor_distances * np.cos(theta_sensors + theta_robot)
    sensor_pos_y = y_robot + sensor_distances * np.sin(theta_sensors + theta_robot)

    # Odczyt intensywności dla każdego czujnika
    sensor_readings = np.array([line_intensity(x, y) for x, y in zip(sensor_pos_x, sensor_pos_y)])

    # Dodanie szumu do odczytów
    sensor_readings = sensor_readings + np.random.normal(0, noise_level, len(sensor_readings))

    sensor_readings = np.clip(sensor_readings, 0, 1)

    # Obliczenie błędu na podstawie średniej ważonej
    if np.sum(sensor_readings) > 0.1:  # Jeśli jakikolwiek czujnik widzi linię
        weights = sensor_readings / np.sum(sensor_readings)
        theta_error = np.sum(np.sin(theta_sensors) * weights)
    else:
        # Jeśli żaden czujnik nie widzi linii, utrzymaj ostatni błąd
        theta_error = last_error if 'last_error' in globals() else 0.0

    return theta_error, sensor_readings



# --- LOGI ---
x_log, y_log = [], []
theta_log = []
theta_err_log = []
wL_log, wR_log = [], []
sensor_readings_log = []
time_log = np.linspace(0, T, N)

# --- PETLA SYMULACJI ---

for i in range(N):
    # Obliczenie błędu i odczytów czujników
    theta_error, sensor_readings = detect_theta_error(x, y, theta)

    # Implementacja kontrolera PID
    integral_error += theta_error * dt
    integral_error = np.clip(integral_error, -1, 1)  # Anti-windup

    derivative_error = (theta_error - last_error) / dt if i > 0 else 0
    last_error = theta_error

    control_signal = Kp * theta_error + Ki * integral_error + Kd * derivative_error

    # Obliczenie napięć na silnikach
    V_cmd = V_bat * 0.5  # Bazowa prędkość
    VL = np.clip(V_cmd + control_signal, 0, V_bat)
    VR = np.clip(V_cmd - control_signal, 0, V_bat)

    # Prad silnikow z uwzględnieniem charakterystyki silnika
    iL = (VL - Ke * wL) / R_motor
    iR = (VR - Ke * wR) / R_motor

    # Moment od silnikow
    TL = Kt * iL
    TR = Kt * iR

    # Uwzglednij tarcie - model tarcia Coulomba z rozróżnieniem statycznego i dynamicznego
    if np.abs(wL) < 0.01:  # Jeśli koło prawie nie obraca się
        friction_L = np.sign(TL) * min(np.abs(TL), mu_friction)
    else:
        friction_L = np.sign(wL) * mu_dynamic

    if np.abs(wR) < 0.01:  # Jeśli koło prawie nie obraca się
        friction_R = np.sign(TR) * min(np.abs(TR), mu_friction)
    else:
        friction_R = np.sign(wR) * mu_dynamic

    TL -= friction_L
    TR -= friction_R

    # Dynamika kol
    alphaL = TL / J_wheel
    alphaR = TR / J_wheel
    wL += alphaL * dt
    wR += alphaR * dt

    # Przeloz na ruch robota
    v = r_wheel * (wL + wR) / 2
    omega = r_wheel * (wR - wL) / L

    # Integracja ruchu robota

    x += v * np.cos(theta) * dt
    y += v * np.sin(theta) * dt
    theta += omega * dt

    # Logowanie

    x_log.append(x)
    y_log.append(y)
    theta_log.append(np.rad2deg(theta))

    theta_err_log.append(np.rad2deg(theta_error))

    wL_log.append(wL)

    wR_log.append(wR)

    sensor_readings_log.append(sensor_readings)



# --- KONWERSJA LOGÓW ---
x_log = np.array(x_log)
y_log = np.array(y_log)
theta_log = np.array(theta_log)
theta_err_log = np.array(theta_err_log)
wL_log = np.array(wL_log)
wR_log = np.array(wR_log)
sensor_readings_log = np.array(sensor_readings_log)

# --- WYGENEROWANIE MAPY TORU ---

def generate_track_map(resolution=0.005):

    x_range = np.arange(-0.4, 0.4, resolution)

    y_range = np.arange(-0.3, 0.3, resolution)

    track_map = np.zeros((len(y_range), len(x_range)))

    

    for i, y in enumerate(y_range):

        for j, x in enumerate(x_range):

            track_map[i, j] = line_intensity(x, y)

    

    return track_map, x_range, y_range



track_map, x_map_range, y_map_range = generate_track_map()



# --- WYKRESY ---

plt.figure(figsize=(15, 10))



# Wykres trajektorii i toru

plt.subplot(2, 2, 1)

plt.imshow(track_map, extent=[min(x_map_range), max(x_map_range), min(y_map_range), max(y_map_range)], 

           origin='lower', cmap='gray_r', alpha=0.7)

plt.plot(x_log, y_log, 'b-', label='Trajektoria robota', linewidth=1.5)



# Rysowanie położeń robota co 2s

sample_rate = int(2 / dt)

for i in range(0, N, sample_rate):

    if i < len(x_log):

        # Rysuj pozycję robota

        robot_x = x_log[i]

        robot_y = y_log[i]

        robot_theta = np.deg2rad(theta_log[i])

        

        # Rysuj robota jako okrąg

        plt.plot(robot_x, robot_y, 'ro', markersize=3)

        

        # Rysuj kierunek robota

        direction_x = robot_x + 0.02 * np.cos(robot_theta)

        direction_y = robot_y + 0.02 * np.sin(robot_theta)

        plt.plot([robot_x, direction_x], [robot_y, direction_y], 'r-')



plt.xlabel('x [m]')
plt.ylabel('y [m]')
plt.title('Trajektoria robota na skomplikowanym torze')
plt.grid(False)
plt.legend()
plt.axis('equal')

# Wykres błędu orientacji
plt.subplot(2, 2, 2)
plt.plot(time_log, theta_err_log)
plt.xlabel('Czas [s]')
plt.ylabel('Błąd kąta [deg]')
plt.title('Błąd orientacji względem linii')
plt.grid(True)

# Wykres prędkości kół
plt.subplot(2, 2, 3)
plt.plot(time_log, wL_log, 'b-', label='Lewe koło')
plt.plot(time_log, wR_log, 'r-', label='Prawe koło')
plt.xlabel('Czas [s]')
plt.ylabel('Prędkość kątowa [rad/s]')
plt.title('Prędkości kół')
plt.legend()
plt.grid(True)

# Wizualizacja odczytów czujników
plt.subplot(2, 2, 4)
plt.imshow(sensor_readings_log.T, aspect='auto', cmap='gray_r', 
           extent=[0, T, -15, 15])  # Używamy kątów w stopniach dla czytelności

plt.xlabel('Czas [s]')
plt.ylabel('Pozycja czujnika [deg]')
plt.title('Odczyty czujników')
plt.colorbar(label='Intensywność')

plt.tight_layout()


# --- ANIMACJA ---

def create_animation():

    fig, ax = plt.subplots(figsize=(8, 6))

    ax.set_xlim(-0.4, 0.4)

    ax.set_ylim(-0.3, 0.3)

    

    # Rysowanie toru (tła)

    ax.imshow(track_map, extent=[min(x_map_range), max(x_map_range), min(y_map_range), max(y_map_range)], 

              origin='lower', cmap='gray_r', alpha=0.7)

    

    # Elementy do aktualizacji

    robot_body = plt.Circle((0, 0), 0.03, fc='lightblue', ec='blue')

    robot_direction = plt.Line2D([0, 0], [0, 0], lw=2, color='red')

    sensor_points = [plt.Circle((0, 0), 0.003, fc='gray') for _ in range(len(theta_sensors))]

    

    # Dodawanie elementów do wykresu

    ax.add_patch(robot_body)

    ax.add_line(robot_direction)

    for sensor in sensor_points:

        ax.add_patch(sensor)

    

    # Tekst informacyjny

    time_text = ax.text(0.02, 0.95, '', transform=ax.transAxes)

    

    # Historia ruchu

    trajectory, = ax.plot([], [], 'b-', lw=1, alpha=0.7)

    

    def init():

        robot_body.center = (x_log[0], y_log[0])

        theta_rad = np.deg2rad(theta_log[0])

        robot_direction.set_data([x_log[0], x_log[0] + 0.04 * np.cos(theta_rad)],

                                [y_log[0], y_log[0] + 0.04 * np.sin(theta_rad)])

        

        for i, sensor in enumerate(sensor_points):

            sensor_theta = theta_sensors[i] + theta_rad

            sensor_x = x_log[0] + sensor_distances[i] * np.cos(sensor_theta)

            sensor_y = y_log[0] + sensor_distances[i] * np.sin(sensor_theta)

            sensor.center = (sensor_x, sensor_y)

            

        time_text.set_text(f'Czas: 0.00s')

        trajectory.set_data([], [])

        return [robot_body, robot_direction, time_text, trajectory] + sensor_points

    

    def update(frame):

        frame = frame * 50  # Rzadszy sampling dla płynności

        if frame < len(x_log):

            robot_body.center = (x_log[frame], y_log[frame])

            theta_rad = np.deg2rad(theta_log[frame])

            robot_direction.set_data([x_log[frame], x_log[frame] + 0.04 * np.cos(theta_rad)],

                                    [y_log[frame], y_log[frame] + 0.04 * np.sin(theta_rad)])

            

            # Rysowanie historii ruchu

            trajectory.set_data(x_log[:frame], y_log[:frame])

            

            for i, sensor in enumerate(sensor_points):

                sensor_theta = theta_sensors[i] + theta_rad

                sensor_x = x_log[frame] + sensor_distances[i] * np.cos(sensor_theta)

                sensor_y = y_log[frame] + sensor_distances[i] * np.sin(sensor_theta)

                sensor.center = (sensor_x, sensor_y)

                

                # Kolor czujnika zależny od odczytu

                intensity = sensor_readings_log[frame, i]

                sensor.set_facecolor(str(1 - intensity))

                

            time_text.set_text(f'Czas: {frame*dt:.2f}s')

        return [robot_body, robot_direction, time_text, trajectory] + sensor_points

    

    # Utwórz animację

    ani = FuncAnimation(fig, update, frames=range(len(x_log) // 50), init_func=init, blit=True, interval=30)
    plt.close(fig)  # Zapobiega wyświetleniu statycznego obrazu
    return ani


# Można teraz bezpiecznie utworzyć i zapisać animację za pomocą:

# fig, anim = create_animation()

# anim.save('line_follower_simulation.gif', writer='pillow', fps=30)

plt.show()