#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    /* Check command-line arguments */
if (argc != 3)
    {
        fprintf(stderr, "Usage: %s input-file output-file\n", argv[0]);
        return 1;
    }

    /* Open input file */
    int input_fd = open(argv[1], O_RDONLY);

    if (input_fd == -1)
    {
        perror("Error opening input file");
        return 1;
    }

    /* Read message from input file */
    char message[BUFFER_SIZE];

    ssize_t bytes_read = read(input_fd, message, BUFFER_SIZE - 1);

    if (bytes_read == -1)
    {
        perror("Error reading input file");
        close(input_fd);
        return 1;
    }

    message[bytes_read] = '\0';

    close(input_fd);

    /* Create two ordinary pipes */
    int pipe_parent_to_child[2];
    int pipe_child_to_parent[2];

    if (pipe(pipe_parent_to_child) == -1)
    {
        perror("Error creating first pipe");
        return 1;
    }

    if (pipe(pipe_child_to_parent) == -1)
    {
        perror("Error creating second pipe");
        close(pipe_parent_to_child[0]);
        close(pipe_parent_to_child[1]);
        return 1;
    }

    /* Create child process */
    pid_t pid = fork();

    if (pid == -1)
    {
        perror("Error creating child process");
        return 1;
    }

    /* =========================
       PROCESS A - PARENT
       ========================= */
    if (pid > 0)
    {
        /* Parent writes to Pipe 1 */
        close(pipe_parent_to_child[0]);

        /* Parent reads from Pipe 2 */
        close(pipe_child_to_parent[1]);

        /* Send original message to child */
        if (write(pipe_parent_to_child[1], message, bytes_read) == -1)
        {
            perror("Error writing to child");
            close(pipe_parent_to_child[1]);
            close(pipe_child_to_parent[0]);
            wait(NULL);
            return 1;
        }

        close(pipe_parent_to_child[1]);

        /* Receive modified message from child */
        char modified_message[BUFFER_SIZE];

        ssize_t modified_bytes =
            read(pipe_child_to_parent[0],
                 modified_message,
                 BUFFER_SIZE - 1);

        if (modified_bytes == -1)
        {
            perror("Error reading from child");
            close(pipe_child_to_parent[0]);
            wait(NULL);
            return 1;
        }

        modified_message[modified_bytes] = '\0';

        close(pipe_child_to_parent[0]);

        /* Open/create output file */
        int output_fd = open(
            argv[2],
            O_WRONLY | O_CREAT | O_TRUNC,
            0644
        );

        if (output_fd == -1)
        {
            perror("Error opening output file");
            wait(NULL);
            return 1;
        }

        /* Write modified message to output file */
        if (write(output_fd, modified_message, modified_bytes) == -1)
        {
            perror("Error writing to output file");
            close(output_fd);
            wait(NULL);
            return 1;
        }

        close(output_fd);

        /* Wait for child to finish */
        wait(NULL);
    }

    /* =========================
       PROCESS B - CHILD
       ========================= */
    
       else
{
    /* Child reads from Pipe 1 */
    close(pipe_parent_to_child[1]);

    /* Child writes to Pipe 2 */
    close(pipe_child_to_parent[0]);

    char child_message[BUFFER_SIZE];

    ssize_t child_bytes =
        read(pipe_parent_to_child[0],
             child_message,
             BUFFER_SIZE - 1);

    if (child_bytes == -1)
    {
        perror("Child error reading from pipe");
        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]);
        return 1;
    }

    /* Switch uppercase to lowercase and lowercase to uppercase */
    for (int i = 0; i < child_bytes; i++)
    {
        if (child_message[i] >= 'a' && child_message[i] <= 'z')
        {
            child_message[i] = child_message[i] - 32;
        }
        else if (child_message[i] >= 'A' && child_message[i] <= 'Z')
        {
            child_message[i] = child_message[i] + 32;
        }
    }

    /* Send modified message back to parent */
    if (write(pipe_child_to_parent[1],
              child_message,
              child_bytes) == -1)
    {
        perror("Child error writing to pipe");
        close(pipe_parent_to_child[0]);
        close(pipe_child_to_parent[1]);
        return 1;
    }

    close(pipe_parent_to_child[0]);
    close(pipe_child_to_parent[1]);
}
    return 0;
}
