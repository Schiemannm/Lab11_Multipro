/**
 * @file mandelmovie.c
 * @brief Generates a Mandelbrot zoom movie by running mandel in parallel.
 * @author Matthew Schiemann
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>

#define DEFAULT_FRAMES 50
#define MANDEL_PATH    "./mandel" // Path of the previous mandel program

static void show_help(void);
static pid_t start_frame(int frame, int nframes, double xcenter, double ycenter,
                         double start_scale, double end_scale,
                         int max_start, int max_end, int width, int height,
                         const char *prefix);
static int reap_child(void);

int main(int argc, char *argv[])
{
    int c;

    // Defaults: zoom into "Seahorse Valley" on the edge of the main cardioid.
    int nprocs = 1;
    int nframes = DEFAULT_FRAMES;
    double xcenter = -0.743643887037151;
    double ycenter = 0.131825904205330;
    double start_scale = 4.0;
    double end_scale = 0.00005;
    int max_start = 200;
    int max_end = 2000;
    int width = 800;
    int height = 800;
    const char *prefix = "mandel";

    // Parse command line options with getopt.
    while ((c = getopt(argc, argv, "p:n:x:y:s:e:m:M:W:H:o:h")) != -1) {
        switch (c) {
            case 'p': 
                nprocs = atoi(optarg); 
                break;
            case 'n': 
                nframes = atoi(optarg); 
                break;
            case 'x':
                xcenter = atof(optarg); 
                break;
            case 'y': 
                ycenter = atof(optarg); 
                break;
            case 's':
                start_scale = atof(optarg);
                break;
            case 'e':
                end_scale = atof(optarg);
                break;
            case 'm':
                max_start = atoi(optarg);
                break;
            case 'M':
                max_end = atoi(optarg);
                break;
            case 'W':
                width = atoi(optarg);
                break;
            case 'H':
                height = atoi(optarg);
                break;
            case 'o':
                prefix = optarg;
                break;
            case 'h':
                show_help();
                return 0;
            default:
                show_help();
                return 1;
        }
    }

    // Validate the settings before creating any processes.
    if (nframes < 1) {
        fprintf(stderr, "mandelmovie: number of frames must be at least 1\n");
        return 1;
    }
    if (nprocs < 1 || nprocs > nframes) {
        fprintf(stderr, "mandelmovie: number of processes must be 1..%d\n", nframes);
        return 1;
    }
    if (start_scale <= 0 || end_scale <= 0) {
        fprintf(stderr, "mandelmovie: scales must be positive\n");
        return 1;
    }
    if (access(MANDEL_PATH, X_OK) != 0) {
        fprintf(stderr, "mandelmovie: cannot run %s (build it with make)\n", MANDEL_PATH);
        return 1;
    }

    printf("mandelmovie: %d frames using %d process(es)\n", nframes, nprocs);

    int running = 0;   // children currently alive
    int failures = 0;  // children that did not exit cleanly

    for (int frame = 0; frame < nframes; frame++) {
        // If the maximum number of children are running, wait for one to
        // finish before starting another.
        if (running == nprocs) {
            failures += reap_child();
            running--;
        }

        if (start_frame(frame, nframes, xcenter, ycenter, start_scale, end_scale,
                        max_start, max_end, width, height, prefix) < 0) {
            failures++;
        } else {
            running++;
        }
    }

    // Wait for all remaining children before exiting.
    while (running > 0) {
        failures += reap_child();
        running--;
    }

    if (failures > 0) {
        fprintf(stderr, "mandelmovie: %d frame(s) failed\n", failures);
        return 1;
    }
    printf("mandelmovie: done\n");
    return 0;
}

/*
Fork one child to render one frame. scale shrinks
Returns the child pid, or -1 if fork failed.
*/
static pid_t start_frame(int frame, int nframes, double xcenter, double ycenter,
                         double start_scale, double end_scale,
                         int max_start, int max_end, int width, int height,
                         const char *prefix)
{
    double t = (nframes > 1) ? (double)frame / (nframes - 1) : 0.0;
    double scale = start_scale * pow(end_scale / start_scale, t);
    int max = max_start + (int)((max_end - max_start) * t);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        // Child: build mandel's arguments as strings and exec it.
        char xs[64], ys[64], ss[64], ms[32], ws[32], hs[32], outfile[256];
        snprintf(xs, sizeof xs, "%.17g", xcenter);
        snprintf(ys, sizeof ys, "%.17g", ycenter);
        snprintf(ss, sizeof ss, "%.17g", scale);
        snprintf(ms, sizeof ms, "%d", max);
        snprintf(ws, sizeof ws, "%d", width);
        snprintf(hs, sizeof hs, "%d", height);
        snprintf(outfile, sizeof outfile, "%s%d.jpg", prefix, frame);

        execl(MANDEL_PATH, "mandel",
              "-x", xs, "-y", ys, "-s", ss, "-m", ms,
              "-W", ws, "-H", hs, "-o", outfile, (char *)NULL);

        // execl only returns on failure.
        perror("execl");
        _exit(127);
    }

    return pid;
}

/*
Wait for any one child to finish. Returns 1 if it failed, 0 if it succeeded.
*/
static int reap_child(void)
{
    int status;
    pid_t pid = wait(&status);
    if (pid < 0) {
        perror("wait");
        return 1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "mandelmovie: child %d failed\n", (int)pid);
        return 1;
    }
    return 0;
}

static void show_help(void)
{
    printf("Use: mandelmovie [options]\n");
    printf("Where options are:\n");
    printf("-p <procs>   Number of child processes to run at once. (default=1)\n");
    printf("-n <frames>  Number of frames to generate. (default=%d)\n", DEFAULT_FRAMES);
    printf("-x <coord>   X coordinate of the zoom target. (default=-0.743643887037151)\n");
    printf("-y <coord>   Y coordinate of the zoom target. (default=0.131825904205330)\n");
    printf("-s <scale>   Scale of the first frame. (default=4)\n");
    printf("-e <scale>   Scale of the last frame. (default=0.00005)\n");
    printf("-m <max>     Max iterations for the first frame. (default=200)\n");
    printf("-M <max>     Max iterations for the last frame. (default=2000)\n");
    printf("-W <pixels>  Width of each frame. (default=800)\n");
    printf("-H <pixels>  Height of each frame. (default=800)\n");
    printf("-o <prefix>  Output file prefix; frames are <prefix>N.jpg. (default=mandel)\n");
    printf("-h           Show this help text.\n");
    printf("\nExample:\n");
    printf("mandelmovie -p 5\n");
    printf("ffmpeg -i mandel%%d.jpg mandel.mpg\n\n");
}