import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt

# Definiujemy parametry robota i toru
K = 1  # Wzmocnienie układu
a = 0.5  # Stała czasowa
L = 0.2  # Rozstaw kół robota
sensor_radius = 0.14  # Odległość listewki czujników od środka obrotu robota
num_sensors = 8  # Liczba czujników
sensor_angles = np.linspace(-np.pi/12, np.pi/12, num_sensors)  # Czujniki ułożone w łuk

# Tworzymy transmitancję w postaci równania różnicowego (scipy nie obsługuje tf bezpośrednio)
numerator = [K]
denominator = [1, a, 0]
G = signal.TransferFunction(numerator, denominator)

# 2️⃣ Strojenie PID metodą Zieglera-Nicholsa (manualne dostrojenie w razie potrzeby)
Kp = -20 * K  # Przykładowe wartości
Ki = 0.0 * K
iKd = 0.075 * K

# Tworzymy filtr PID w dziedzinie czasu (regulator cyfrowy)
def pid_control(error, dt, integral, previous_error):
    integral += error * dt
    derivative = (error - previous_error) / dt
    output = Kp * error + Ki * integral + iKd * derivative
    return output, integral, error

# 3️⃣ Symulacja ruchu robota po eliptycznym torze z listewką czujników

def simulate_robot(kp, ki, kd, total_time=10, dt=0.01):
    t_values = np.arange(0, total_time, dt)
    x, y, theta = np.cos(0), 0.5 * np.sin(0), np.pi/2  # Początkowe położenie na torze
    integral = 0
    previous_error = 0
    path_x, path_y = [], []
    sensor_readings = []

    for t in t_values:
        desired_x = np.cos(t)  # Tor eliptyczny
        desired_y = 0.5 * np.sin(t)
        
        sensor_positions = [
            (x + sensor_radius * np.cos(theta + angle), y + sensor_radius * np.sin(theta + angle))
            for angle in sensor_angles
        ]
        
        sensor_errors = [
            (desired_x - sx) * np.sin(theta) - (desired_y - sy) * np.cos(theta)
            for sx, sy in sensor_positions
        ]
        
        avg_sensor_error = np.mean(sensor_errors)  # Średnia wartość błędu z czujników
        control_signal, integral, previous_error = pid_control(avg_sensor_error, dt, integral, previous_error)
        omega = control_signal / L  # Sterowanie kątem skrętu
        theta += omega * dt
        x += np.cos(theta) * dt
        y += np.sin(theta) * dt
        
        path_x.append(x)
        path_y.append(y)
        sensor_readings.append(avg_sensor_error)

    return path_x, path_y, sensor_readings

# 4️⃣ Wizualizacja ruchu robota
robot_x, robot_y, sensor_data = simulate_robot(Kp, Ki, iKd)

plt.figure(figsize=(10, 5))
plt.subplot(1, 2, 1)
plt.plot(robot_x, robot_y, label="Trajektoria robota")
plt.plot(np.cos(np.arange(0, 10, 0.1)), 0.5 * np.sin(np.arange(0, 10, 0.1)), '--', label="Tor eliptyczny")
plt.xlabel("X")
plt.ylabel("Y")
plt.title("Symulacja Line Followera - PID")
plt.legend()
plt.grid()

plt.subplot(1, 2, 2)
plt.plot(sensor_data, label="Średni błąd czujników")
plt.xlabel("Czas (s)")
plt.ylabel("Błąd czujników")
plt.title("Błąd czujników podczas jazdy")
plt.legend()
plt.grid()

plt.show()

# Wydrukowane wartości PID
print(f"Dostrojone parametry PID: Kp={Kp}, Ki={Ki}, Kd={iKd}")