import pygame
import serial
import random
import math
import os

# --- HARDWARE SETTINGS ---
COM_PORT = 'COM7'  # <-- CHANGE TO YOUR PORT!
BAUD_RATE = 115200

# --- GAME SETTINGS ---
WIDTH, HEIGHT = 1024, 768
FPS = 60
TARGET_RADIUS = 30

# Colors (RGB)
BLACK = (15, 15, 15)
WHITE = (255, 255, 255)
RED = (220, 20, 60)
GREEN = (50, 205, 50)

# --- INITIALIZATION ---
pygame.init()
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("STM32 Hardware Aim Trainer")
clock = pygame.time.Clock()

# Variables initialization
crosshair_x, crosshair_y = WIDTH // 2, HEIGHT // 2
target_x = random.randint(100, WIDTH - 100)
target_y = random.randint(100, HEIGHT - 100)

score = 0
highscore = 0
button_was_pressed = False

# Load High Score from file
if os.path.exists("highscore.txt"):
    with open("highscore.txt", "r") as f:
        try:
            highscore = int(f.read())
        except ValueError:
            pass

# Open serial port
try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=0.01)
    print(f"Connected to {COM_PORT}")
except Exception as e:
    print(f"ERROR: Cannot open port {COM_PORT}. Check the name and close RealTerm!")
    exit()

# --- MAIN GAME LOOP ---
running = True
while running:
    # 1. System events handling (e.g., closing window)
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    # 2. READ DATA FROM STM32 (UART)
    if ser.in_waiting > 0:
        try:
            # Read raw line, decode and remove whitespaces
            line = ser.readline().decode('utf-8').strip()

            # Expected STM32 format: "X: 2048 | Y: 2048 | BTN: 0"
            if line.startswith("X:"):
                # Split text into parts (Parsing)
                parts = line.split('|')
                raw_x = int(parts[0].split(':')[1].strip())
                raw_y = int(parts[1].split(':')[1].strip())
                btn = int(parts[2].split(':')[1].strip())

                # MAPPING: Convert ADC range (0 - 4095) to screen pixels
                crosshair_x = int((raw_x / 4095) * WIDTH)

                # Invert Y axis because in Pygame 0 is at the top of the screen
                crosshair_y = int(((4095 - raw_y) / 4095) * HEIGHT)

                # SHOOTING MECHANICS (Rising edge detection)
                if btn == 1 and not button_was_pressed:

                    # Math: Pythagorean theorem to measure distance
                    dist = math.hypot(crosshair_x - target_x, crosshair_y - target_y)

                    if dist <= TARGET_RADIUS:
                        # Hit!
                        score += 1
                        if score > highscore:
                            highscore = score

                        # Generate new target
                        target_x = random.randint(100, WIDTH - 100)
                        target_y = random.randint(100, HEIGHT - 100)

                # Update button memory (prevents "full-auto" shooting when held down)
                button_was_pressed = (btn == 1)

        except Exception as e:
            # Ignore corrupted frames (e.g., first frame after connecting cable)
            pass

            # 3. DRAW GRAPHICS
    screen.fill(BLACK)  # Clear screen (new frame)

    # Draw target
    pygame.draw.circle(screen, RED, (target_x, target_y), TARGET_RADIUS)

    # Draw player crosshair (circle + 4 lines)
    pygame.draw.circle(screen, GREEN, (crosshair_x, crosshair_y), 4)
    pygame.draw.line(screen, GREEN, (crosshair_x - 15, crosshair_y), (crosshair_x + 15, crosshair_y), 2)
    pygame.draw.line(screen, GREEN, (crosshair_x, crosshair_y - 15), (crosshair_x, crosshair_y + 15), 2)

    # Draw UI text
    font = pygame.font.SysFont(None, 42)
    score_text = font.render(f"Hits: {score}    |    High Score: {highscore}", True, WHITE)
    screen.blit(score_text, (20, 20))

    pygame.display.flip()  # Push new frame to monitor
    clock.tick(FPS)  # Maintain stable 60 FPS

# --- PROGRAM TERMINATION ---
# Save highest score to a text file
with open("highscore.txt", "w") as f:
    f.write(str(highscore))

ser.close()
pygame.quit()
print("Game closed. Score saved!")