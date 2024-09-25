import socket
import threading
import tkinter as tk
from tkinter import ttk

HOST = '0.0.0.0'
PORT = 4242

exercises = ["Pause", "Left-right", "Up-down", "Front-back", "Circle"]

total_number = 0
correct_number = 0
execution_time = 10

# Interval de timeout în secunde
timeout_interval = 5

class ServerApp:
    def __init__(self, master):
        self.master = master
        self.master.title("Pico W Data Viewer")

        main_frame = ttk.Frame(self.master, padding="10")
        main_frame.grid(row=0, column=0, sticky="nsew")

        self.master.grid_rowconfigure(0, weight=1)
        self.master.grid_columnconfigure(0, weight=1)
        main_frame.grid_rowconfigure(0, weight=1)
        main_frame.grid_columnconfigure(0, weight=1)

        select_label = ttk.Label(main_frame, text="Select hand and exercise", font=("Helvetica", 16, "bold"))
        select_label.grid(row=0, column=0, columnspan=8, pady=10)

        for exercise_type in exercises:
            self.create_exercise_row(main_frame, exercises.index(exercise_type) + 1, exercise_type,
                                     exercise_type.lower().replace("-", "_"))

        self.timers = {}

        bottom_frame = ttk.Frame(self.master, padding="10")
        bottom_frame.grid(row=1, column=0, sticky="ew")

        self.master.grid_rowconfigure(1, weight=0)

        time_label = ttk.Label(bottom_frame, text="Set Execution Time (s):", font=("Helvetica", 12))
        time_label.grid(row=0, column=0, padx=10, pady=10, sticky="e")

        self.time_entry = ttk.Entry(bottom_frame, width=5)
        self.time_entry.grid(row=0, column=1, padx=10, pady=10, sticky="w")
        self.time_entry.insert(0, str(execution_time))

        set_time_button = ttk.Button(bottom_frame, text="Set Time", command=self.set_execution_time)
        set_time_button.grid(row=0, column=2, padx=10, pady=10, sticky="e")

        right_frame = ttk.Frame(self.master, padding="10")
        right_frame.grid(row=1, column=1, sticky="se")
        self.master.grid_columnconfigure(1, weight=0)

        connected_label = ttk.Label(right_frame, text="Connected hand:", font=("Helvetica", 12))
        connected_label.grid(row=0, column=0, padx=10, pady=10, sticky="e")

        self.left_connection_bulb = tk.Canvas(right_frame, width=20, height=20)
        self.left_connection_bulb.grid(row=0, column=1, padx=10, pady=10)
        self.update_bulb(self.left_connection_bulb, "gray")

        self.right_connection_bulb = tk.Canvas(right_frame, width=20, height=20)
        self.right_connection_bulb.grid(row=0, column=2, padx=10, pady=10)
        self.update_bulb(self.right_connection_bulb, "gray")

        # Inițializează timerele de timeout
        self.left_hand_timer = None
        self.right_hand_timer = None

        self.server_thread = threading.Thread(target=self.run_server)
        self.server_thread.daemon = True
        self.server_thread.start()

    def create_exercise_row(self, parent, row, label_text, exercise_type):
        label = ttk.Label(parent, text=label_text, font=("Helvetica", 14))
        label.grid(row=row, column=0, padx=10, pady=10, sticky="w")

        left_var = tk.BooleanVar()
        right_var = tk.BooleanVar()
        left_check = ttk.Checkbutton(parent, text="Left Hand", variable=left_var,
                                     command=lambda: self.toggle_exercise(exercise_type))
        right_check = ttk.Checkbutton(parent, text="Right Hand", variable=right_var,
                                      command=lambda: self.toggle_exercise(exercise_type))
        left_check.grid(row=row, column=1, padx=10, pady=10)
        right_check.grid(row=row, column=2, padx=10, pady=10)

        setattr(self, f"{exercise_type}_left_var", left_var)
        setattr(self, f"{exercise_type}_right_var", right_var)

        timer_label = ttk.Label(parent, text=f"Time: {execution_time} s", font=("Helvetica", 16))
        timer_label.grid(row=row, column=3, padx=10, pady=10)
        setattr(self, f"{exercise_type}_timer_label", timer_label)
        setattr(self, f"{exercise_type}_time_remaining", execution_time)

        left_bulb = tk.Canvas(parent, width=20, height=20)
        right_bulb = tk.Canvas(parent, width=20, height=20)
        left_bulb.grid(row=row, column=4, pady=10)
        right_bulb.grid(row=row, column=5, pady=10)
        setattr(self, f"{exercise_type}_left_bulb", left_bulb)
        setattr(self, f"{exercise_type}_right_bulb", right_bulb)
        self.update_bulb(left_bulb, "gray")
        self.update_bulb(right_bulb, "gray")

        success_rate_var = tk.DoubleVar(value=0.0)
        success_rate_label = ttk.Label(parent, text="Success Rate:", font=("Helvetica", 14))
        success_rate_label.grid(row=row, column=6, padx=10, pady=10, sticky="e")
        success_rate_value = ttk.Label(parent, text=f"{success_rate_var.get():.2f} %", font=("Helvetica", 14))
        success_rate_value.grid(row=row, column=7, padx=10, pady=10, sticky="w")
        setattr(self, f"{exercise_type}_success_rate_var", success_rate_var)
        setattr(self, f"{exercise_type}_success_rate_value", success_rate_value)

    def toggle_exercise(self, exercise_type):
        for other_exercise_type in ["left_right", "up_down", "front_back", "pause"]:
            if other_exercise_type != exercise_type:
                self.reset_exercise(other_exercise_type, reset_selection=True)

        left_var = getattr(self, f"{exercise_type}_left_var")
        right_var = getattr(self, f"{exercise_type}_right_var")
        if left_var.get() or right_var.get():
            self.start_timer(exercise_type)
        else:
            self.reset_exercise(exercise_type, reset_selection=False)

    def start_timer(self, exercise_type):
        if getattr(self, f"{exercise_type}_time_remaining") == execution_time:
            self.update_timer(exercise_type)

    def reset_exercise(self, exercise_type, reset_selection):
        global correct_number
        global total_number

        correct_number = 0
        total_number = 0
        setattr(self, f"{exercise_type}_time_remaining", execution_time)
        timer_label = getattr(self, f"{exercise_type}_timer_label")
        timer_label.config(text=f"Time: {execution_time} s")

        if exercise_type in self.timers:
            self.master.after_cancel(self.timers[exercise_type])
            del self.timers[exercise_type]

        left_bulb = getattr(self, f"{exercise_type}_left_bulb")
        right_bulb = getattr(self, f"{exercise_type}_right_bulb")
        self.update_bulb(left_bulb, "gray")
        self.update_bulb(right_bulb, "gray")

        success_rate_var = getattr(self, f"{exercise_type}_success_rate_var")
        success_rate_var.set(0.0)

        left_var = getattr(self, f"{exercise_type}_left_var")
        right_var = getattr(self, f"{exercise_type}_right_var")
        left_var.set(False)
        right_var.set(False)

    def update_timer(self, exercise_type):
        time_remaining = getattr(self, f"{exercise_type}_time_remaining")
        if time_remaining > 0:
            time_remaining -= 1
            setattr(self, f"{exercise_type}_time_remaining", time_remaining)
            timer_label = getattr(self, f"{exercise_type}_timer_label")
            timer_label.config(text=f"Time: {time_remaining} s")
            self.timers[exercise_type] = self.master.after(1000, self.update_timer, exercise_type)
        else:
            timer_label = getattr(self, f"{exercise_type}_timer_label")
            timer_label.config(text="Time's up!")

            exercise_type_lower = exercise_type.lower().replace("-", "_")
            success_rate_var = getattr(self, f"{exercise_type_lower}_success_rate_var")
            success_rate_value = getattr(self, f"{exercise_type_lower}_success_rate_value")

            if total_number > 0:
                success_rate = (correct_number / total_number) * 100
            else:
                success_rate = 0.0

            success_rate_var.set(success_rate)
            success_rate_value.config(text=f"{success_rate:.2f} %")

            left_bulb = getattr(self, f"{exercise_type}_left_bulb")
            right_bulb = getattr(self, f"{exercise_type}_right_bulb")
            self.update_bulb(left_bulb, "gray")
            self.update_bulb(right_bulb, "gray")

            left_var = getattr(self, f"{exercise_type}_left_var")
            right_var = getattr(self, f"{exercise_type}_right_var")
            left_var.set(False)
            right_var.set(False)

    def set_execution_time(self):
        global execution_time
        try:
            new_time = int(self.time_entry.get())
            execution_time = new_time
            for exercise_type in exercises:
                exercise_type_lower = exercise_type.lower().replace("-", "_")
                setattr(self, f"{exercise_type_lower}_time_remaining", execution_time)
                timer_label = getattr(self, f"{exercise_type_lower}_timer_label")
                timer_label.config(text=f"Time: {execution_time}")
        except ValueError:
            print("Invalid time value")

    def run_server(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.bind((HOST, PORT))
            s.listen()
            print(f"Server listening on {HOST}:{PORT}")

            while True:
                conn, addr = s.accept()
                thread = threading.Thread(target=self.handle_client, args=(conn, addr))
                thread.start()

    def handle_client(self, conn, addr):
        print(f"Connected by {addr}")

        while True:
            data = conn.recv(1024)
            if not data:
                break
            message = data.decode()
            print(f"Received from {addr}: {message}")
            self.process_message(addr, message)

        print(f"Disconnected from {addr}")
        conn.close()

    def process_message(self, addr, message):
        global total_number
        global correct_number
        try:
            hand, received_exercise = message.split('->')
            hand = hand.strip().lower()
            received_exercise = received_exercise.strip().lower().replace("-", "_")

            for exercise_type in exercises:
                exercise_type_lower = exercise_type.lower().replace("-", "_")
                left_var = getattr(self, f"{exercise_type_lower}_left_var")
                right_var = getattr(self, f"{exercise_type_lower}_right_var")
                left_bulb = getattr(self, f"{exercise_type_lower}_left_bulb")
                right_bulb = getattr(self, f"{exercise_type_lower}_right_bulb")

                if hand == "stanga":
                    if left_var.get():
                        correct_exercise = exercise_type_lower == received_exercise
                        total_number += 1
                        if correct_exercise:
                            correct_number += 1
                        self.update_bulb(left_bulb, "green" if correct_exercise else "red")
                    else:
                        self.update_bulb(left_bulb, "gray")
                elif hand == "dreapta":
                    if right_var.get():
                        correct_exercise = exercise_type_lower == received_exercise
                        total_number += 1
                        if correct_exercise:
                            correct_number += 1
                        self.update_bulb(right_bulb, "green" if correct_exercise else "red")
                    else:
                        self.update_bulb(right_bulb, "gray")

            # Resetează timerul de timeout pentru mâna respectivă
            if hand == "stanga":
                if self.left_hand_timer:
                    self.master.after_cancel(self.left_hand_timer)
                self.left_hand_timer = self.master.after(timeout_interval * 1000, self.timeout_hand, "stanga")
                self.update_bulb(self.left_connection_bulb, "green")
            elif hand == "dreapta":
                if self.right_hand_timer:
                    self.master.after_cancel(self.right_hand_timer)
                self.right_hand_timer = self.master.after(timeout_interval * 1000, self.timeout_hand, "dreapta")
                self.update_bulb(self.right_connection_bulb, "green")

        except ValueError:
            print(f"Invalid message format: {message}")

    def timeout_hand(self, hand):
        if hand == "stanga":
            self.update_bulb(self.left_connection_bulb, "red")
        elif hand == "dreapta":
            self.update_bulb(self.right_connection_bulb, "red")

    def update_bulb(self, bulb, color):
        bulb.delete("all")
        bulb.create_oval(5, 5, 20, 20, fill=color)


def main():
    root = tk.Tk()
    app = ServerApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
