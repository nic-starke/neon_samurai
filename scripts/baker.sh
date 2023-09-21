#!/bin/sh

# Define the commands and their corresponding functions
commands="build clean info configure meson logs exit"
functions="\
meson compile -C build;\
echo 'Clean command not implemented';\
echo 'Info command not implemented';\
echo 'Configure command not implemented';\
meson setup --cross-file cross-file.txt build;\
echo 'Logs command not implemented';\
exit"

# Define the function to execute a command
execute_command() {
    output=$(eval "$1" 2>&1)
    echo "$output"
    printf "Press enter to continue"
    read dummy
}

# Main loop
current_command=0
while true; do
    # Print the menu
    clear
    echo "Select a command:"
    i=0
    for cmd in $commands; do
        if [ $i -eq $current_command ]; then
            echo "> $cmd"
        else
            echo "  $cmd"
        fi
        i=$((i + 1))
    done

    # Get user input
    read -rsn1 input
    if [ "$input" = "" ]; then
        # Execute the selected command
        cmd=$(echo "$commands" | awk -v n=$current_command 'BEGIN {RS=" "} NR==n+1 {print}')
        if [ "$cmd" = "exit" ]; then
            break
        else
            func=$(echo "$functions" | awk -v n=$current_command 'BEGIN {RS=";"} NR==n+1 {print}')
            execute_command "$func"
        fi
    elif [ "$input" = $'\x1b' ]; then
        # Handle arrow keys
        read -rsn2 input
        if [ "$input" = $'\x1b[A' ]; then
            current_command=$(( (current_command - 1 + $(echo "$commands" | wc -w)) % $(echo "$commands" | wc -w) ))
        elif [ "$input" = $'\x1b[B' ]; then
            current_command=$(( (current_command + 1) % $(echo "$commands" | wc -w) ))
        fi
    fi

    echo $current_command
done