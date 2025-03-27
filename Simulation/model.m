% Parametry robota
L = 0.1; % Rozstaw kół
v = 1;   % Prędkość liniowa

% Macierze modelu stanu
A = [0 1 0; 0 0 v; 0 0 0];  
B = [0; 0; 1/L];  
C = eye(3);  
D = [0; 0; 0];

% Tworzymy model state-space
sys = ss(A, B, C, D);

% Strojenie PID
C_pid = pidtune(sys, 'PID')

% Symulacja odpowiedzi skokowej
step(sys)
grid on

