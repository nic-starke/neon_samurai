import curses
import curses.panel
import subprocess

# Define the commands and their corresponding functions
commands = [
    {"name": "build", "function": 'meson compile -C build'},
    {"name": "clean", "function": NotImplemented},
    {"name": "info", "function": NotImplemented},
    {"name": "configure", "function": NotImplemented},
    {"name": "meson", "function": 'meson setup --cross-file cross-file.txt build'},
    {"name": "logs", "function": NotImplemented},
    {"name": "exit", "function": exit},
]


def main(stdscr):
    def execute_command(cmd):
        nonlocal output_pad
        process = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False, text=True)
        stdout, stderr = process.communicate()
        output_pad.addstr(stdout)
        output_pad.addstr(stderr)
        output_pad.noutrefresh(0, 0, 2, max_command_width + 1, curses.LINES - 1, curses.COLS - 1)
        
    curses.curs_set(0)
    stdscr.refresh()

    max_command_width = max(len(cmd['name']) for cmd in commands) + 4

    # Create left panel for commands
    left_panel_win = curses.newwin(curses.LINES, max_command_width, 0, 0)
    left_panel = curses.panel.new_panel(left_panel_win)

    # Create right panel for output
    right_panel_win = curses.newwin(curses.LINES, curses.COLS - max_command_width, 0, max_command_width)
    right_panel = curses.panel.new_panel(right_panel_win)

    # Create output pad
    output_pad = curses.newpad(10000, curses.COLS - max_command_width - 4)
    output_pad.scrollok(1)

    left_panel_focus = True  # Initial focus on the left panel
    right_panel_focus = False

    current_command = 0

    # Main loop
    while True:
        
        # Update the command list on the left panel
        for i, cmd in enumerate(commands):
            if i == current_command:
                left_panel_win.addstr(i + 1, 1, cmd['name'], curses.A_REVERSE)
            else:
                left_panel_win.addstr(i + 1, 1, cmd['name'])
        left_panel_win.refresh()

        key = stdscr.getch()
        if key == ord('q') or key == 27:  # Exit on 'q' or ESC
            break
        elif key == curses.KEY_UP:
            current_command = (current_command - 1) % len(commands)
        elif key == curses.KEY_DOWN:
            current_command = (current_command + 1) % len(commands)
        elif key == ord('\n'):
            cmd = commands[current_command]
            if cmd['function'] == NotImplemented:
                output_pad.addstr("Not implemented\n")
                output_pad.addstr("---\n")
            else:
                execute_command(cmd['function'])
                output_pad.addstr("---\n")
                output_pad.noutrefresh(0, 0, 2, max_command_width + 1, curses.LINES - 1, curses.COLS - 1)
        elif key == curses.KEY_PPAGE:  # Page up
            output_pad.scroll(-curses.LINES)
        elif key == curses.KEY_NPAGE:  # Page down
            output_pad.scroll(curses.LINES)
    
        # Refresh the panels
        output_pad.refresh(0, 0, 2, max_command_width + 1, curses.LINES - 1, curses.COLS - 1)
        curses.panel.update_panels()
        curses.doupdate()

if __name__ == "__main__":
    curses.wrapper(main)
