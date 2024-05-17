#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>

#define MAX_POINTS 4096
#define MAX_CLUSTERS 32
#define MAX_THREADS 32

typedef struct point {
    float x;      // The x-coordinate of the point
    float y;      // The y-coordinate of the point
    int cluster;  // The cluster that the point belongs to
} point;

int N;                  // number of entries in the data
int k;                  // number of centroids
int num_threads;        // number of threads
point data[MAX_POINTS]; // Data coordinates
point cluster[MAX_CLUSTERS]; // The coordinates of each cluster center (also called centroid)

/* pthread variables */
pthread_t threads[MAX_THREADS];
pthread_barrier_t barrier;

/* forward declarations */
void *kmeans_parallel(void *arg);
void read_data();
int get_closest_centroid(int i, int k);
bool assign_clusters_to_points();
void update_cluster_centers();
void write_results();

int main() {
    printf("Parallel K-Means Clustering\n");

    read_data();
    pthread_barrier_init(&barrier, NULL, num_threads + 1);

    for (int i = 0; i < num_threads; i++) {
        int *thread_id = (int *)malloc(sizeof(int));
        *thread_id = i;
        pthread_create(&threads[i], NULL, kmeans_parallel, (void *)thread_id);
    }

    pthread_barrier_wait(&barrier);

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&barrier);

    write_results();

    return 0;
}

void *kmeans_parallel(void *arg) {
    int thread_id = *((int *)arg);

    int iter = 0;
    bool somechange;

    do {
        iter++;
        somechange = false;

        for (int i = thread_id; i < N; i += num_threads) {
            int old_cluster = data[i].cluster;
            int new_cluster = get_closest_centroid(i, k);
            data[i].cluster = new_cluster;

            if (old_cluster != new_cluster) {
                somechange = true;
            }
        }

        pthread_barrier_wait(&barrier);

        if (thread_id == 0) {
            update_cluster_centers();
        }

        pthread_barrier_wait(&barrier);
    } while (somechange);

    if (thread_id == 0) {
        printf("Number of iterations taken = %d\n", iter);
        printf("Computed cluster numbers successfully!\n");
    }

    free(arg);
    return NULL;
}

void read_data()
{
    N = 1797;
    k = 9;
    FILE* fp = fopen("kmeans-data.txt", "r");
    if (fp == NULL) {
        perror("Cannot open the file");
        exit(EXIT_FAILURE);
    }
   
    // Initialize points from the data file
    //float temp;
    for (int i = 0; i < N; i++)
    {
        fscanf(fp, "%f %f", &data[i].x, &data[i].y);
        data[i].cluster = -1; // Initialize the cluster number to -1
    }
    printf("Read the problem data!\n");
    // Initialize centroids randomly
    srand(0); // Setting 0 as the random number generation seed
    for (int i = 0; i < k; i++)
    {
        int r = rand() % N;
        cluster[i].x = data[r].x;
        cluster[i].y = data[r].y;
    }
    fclose(fp);
}

int get_closest_centroid(int i, int k)
{
    /* find the nearest centroid */
    int nearest_cluster = -1;
    double xdist, ydist, dist, min_dist;
    min_dist = dist = INT_MAX;
    for (int c = 0; c < k; c++)
    { // For each centroid
        // Calculate the square of the Euclidean distance between that centroid and the point
        xdist = data[i].x - cluster[c].x;
        ydist = data[i].y - cluster[c].y;
        dist = xdist * xdist + ydist * ydist; // The square of Euclidean distance
        //printf("%.2lf \n", dist);
        if (dist <= min_dist)
        {
            min_dist = dist;
            nearest_cluster = c;
        }
    }
    //printf("-----------\n");
    return nearest_cluster;
}

bool assign_clusters_to_points()
{
    bool something_changed = false;
    int old_cluster = -1, new_cluster = -1;
    for (int i = 0; i < N; i++)
    { // For each data point
        old_cluster = data[i].cluster;
        new_cluster = get_closest_centroid(i, k);
        data[i].cluster = new_cluster; // Assign a cluster to the point i
        if (old_cluster != new_cluster)
        {
            something_changed = true;
        }
    }
    return something_changed;
}

void update_cluster_centers() {
    int c;
    int count[MAX_CLUSTERS] = {0}; // Array to keep track of the number of points in each cluster
    point temp[MAX_CLUSTERS] = {{0.0, 0.0, 0}};  // Initialize with {0.0, 0.0, 0}

    for (int i = 0; i < N; i++) {
        c = data[i].cluster;
        count[c]++;
        temp[c].x += data[i].x;
        temp[c].y += data[i].y;
    }

    for (int i = 0; i < MAX_CLUSTERS; i++) {
        // Check if count[i] is not zero to avoid division by zero
        if (count[i] != 0) {
            cluster[i].x = temp[i].x / count[i];
            cluster[i].y = temp[i].y / count[i];
        }
    }
}

/*int kmeans() {
    bool somechange;
    int iter = 0;
    do {
        iter++; // Keep track of the number of iterations
        somechange = assign_clusters_to_points();
        update_cluster_centers();
    } while (somechange);

    printf("Number of iterations taken = %d\n", iter);
    printf("Computed cluster numbers successfully!\n");
    return 0;
}*/

void write_results()
{
    FILE* fp = fopen("kmeans-results.txt", "w");
    if (fp == NULL) {
        perror("Cannot open the file");
        exit(EXIT_FAILURE);
    }
    else
    {
        for (int i = 0; i < N; i++)
        {
            fprintf(fp, "%.2f %.2f %d\n", data[i].x, data[i].y, data[i].cluster);
        }
    }
    printf("Wrote the results to a file!\n");
}


