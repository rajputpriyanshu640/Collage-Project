#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define DRIVER_DB_FILE "drivers.txt"
#define MAX_NODES 20
#define NODE_COUNT 12
#define INF INT_MAX/4
#define RATE_PER_UNIT 5.0
#define MAX_NAME 50

typedef enum { AVAILABLE, ON_RIDE, OFFLINE } DriverStatus;

typedef struct Driver {
    int id;
    char name[MAX_NAME];
    DriverStatus status;
    double earnings;
    int location;
} Driver;

typedef struct RideRequest {
    int id;
    char passengerName[MAX_NAME];
    int otp;
    int pickup;
    int drop;
} RideRequest;

typedef struct DriverNode {
    Driver driver_data;
    struct DriverNode* next;
} DriverNode;

typedef struct RideRequestNode {
    RideRequest request_data;
    struct RideRequestNode* next;
} RideRequestNode;

typedef struct Queue {
    RideRequestNode *front, *rear;
} Queue;


// --------------------------------------------
// QUEUE SYSTEM
// --------------------------------------------

Queue* createQueue() {
    Queue* q = malloc(sizeof(Queue));
    if (!q) return NULL;
    q->front = q->rear = NULL;
    return q;
}

void enqueue(Queue* q, RideRequest new_request) {
    if (!q) return;
    RideRequestNode* n = malloc(sizeof(RideRequestNode));
    if (!n) return;
    n->request_data = new_request;
    n->next = NULL;

    if (!q->rear) {
        q->front = q->rear = n;
        return;
    }
    q->rear->next = n;
    q->rear = n;
}

RideRequest* dequeue(Queue* q) {
    if (!q || !q->front) return NULL;

    RideRequestNode* temp = q->front;
    RideRequest* out = malloc(sizeof(RideRequest));
    if (!out) return NULL;
    *out = temp->request_data;

    q->front = q->front->next;
    if (!q->front) q->rear = NULL;

    free(temp);
    return out;
}


// --------------------------------------------
// FILE HANDLING
// --------------------------------------------

void saveDriversToFile(DriverNode* head) {
    FILE* f = fopen(DRIVER_DB_FILE, "w");
    if (!f) return;

    for (DriverNode* c = head; c; c=c->next) {
        fprintf(f, "%d,%s,%d,%.2f,%d\n",
            c->driver_data.id,
            c->driver_data.name,
            c->driver_data.status,
            c->driver_data.earnings,
            c->driver_data.location
        );
    }
    fclose(f);
}

void loadDriversFromFile(DriverNode** headRef) {
    FILE* f = fopen(DRIVER_DB_FILE, "r");
    if (!f) return;

    int id, status_int, loc;
    double earn;
    char namebuf[128];

    while (fscanf(f, "%d,%127[^,],%d,%lf,%d\n",
        &id, namebuf, &status_int, &earn, &loc) == 5)
    {
        DriverNode* n = malloc(sizeof(DriverNode));
        if (!n) break;
        n->driver_data.id = id;
        strncpy(n->driver_data.name, namebuf, MAX_NAME-1);
        n->driver_data.name[MAX_NAME-1] = '\0';
        n->driver_data.status = (DriverStatus)status_int;
        n->driver_data.earnings = earn;
        n->driver_data.location = loc;

        n->next = *headRef;
        *headRef = n;
    }

    fclose(f);
}


// --------------------------------------------
// DRIVER MANAGEMENT
// --------------------------------------------

void addDriver(DriverNode** headRef) {
    DriverNode* n = malloc(sizeof(DriverNode));
    if (!n) { printf("[ERROR] Memory allocation failed.\n"); return; }
    char buf[100];

    printf("Enter driver name: ");
    if (!fgets(buf, sizeof(buf), stdin)) { free(n); return; }
    buf[strcspn(buf,"\n")] = 0;

    if (buf[0] == '\0') {
        printf("[ERROR] Name cannot be empty.\n");
        free(n);
        return;
    }

    strncpy(n->driver_data.name, buf, MAX_NAME-1);
    n->driver_data.name[MAX_NAME-1] = '\0';
    n->driver_data.id = rand()%9000 + 1000;
    n->driver_data.status = AVAILABLE;
    n->driver_data.earnings = 0;
    n->driver_data.location = rand()%NODE_COUNT + 1; /* 1..NODE_COUNT */

    n->next = *headRef;
    *headRef = n;

    printf("[SUCCESS] %s added at node %d (ID: %d)\n",
        n->driver_data.name,
        n->driver_data.location,
        n->driver_data.id);
}

const char* getStatusString(DriverStatus s) {
    if (s==AVAILABLE) return "Available";
    if (s==ON_RIDE) return "On Ride";
    return "Offline";
}

void viewAllDrivers(DriverNode* head) {
    if (!head) {
        printf("No drivers.\n");
        return;
    }

    for (DriverNode* c=head; c; c=c->next) {
        printf("ID:%d | Name:%s | Status:%s | Earnings:%.2f | Loc:%d\n",
            c->driver_data.id,
            c->driver_data.name,
            getStatusString(c->driver_data.status),
            c->driver_data.earnings,
            c->driver_data.location
        );
    }
}

void updateDriverStatus(DriverNode* head) {
    char b[20];
    printf("Enter driver ID: ");
    if (!fgets(b, sizeof(b), stdin)) return;
    int id = atoi(b);

    for (DriverNode* c=head; c; c=c->next) {
        if (c->driver_data.id == id) {

            if (c->driver_data.status == ON_RIDE) {
                printf("[ERROR] Cannot change status. Driver on ride.\n");
                return;
            }

            printf("1. Available\n2. Offline\nChoice: ");
            if (!fgets(b, sizeof(b), stdin)) return;
            int ch = atoi(b);

            if (ch == 1) c->driver_data.status = AVAILABLE;
            else if (ch == 2) c->driver_data.status = OFFLINE;
            else printf("[ERROR] Invalid choice.\n");

            return;
        }
    }

    printf("[ERROR] Driver not found.\n");
}

void removeDriver(DriverNode** headRef) {
    char b[20];
    printf("Enter driver ID: ");
    if (!fgets(b, sizeof(b), stdin)) return;
    int id = atoi(b);

    DriverNode *c=*headRef, *p=NULL;

    while (c) {
        if (c->driver_data.id == id) {
            if (p) p->next = c->next; 
            else *headRef = c->next;
            free(c);
            printf("[SUCCESS] Driver removed.\n");
            return;
        }
        p=c;
        c=c->next;
    }

    printf("[ERROR] Driver not found.\n");
}


// --------------------------------------------
// GRAPH + DIJKSTRA
// --------------------------------------------

void initGraph(int graph[MAX_NODES][MAX_NODES], int *node_count) {
    int n = NODE_COUNT;
    *node_count = n;

    for (int i=0;i<MAX_NODES;i++)
        for (int j=0;j<MAX_NODES;j++)
            graph[i][j] = (i==j ? 0 : INF);

#define E(a,b,w) graph[a][b]=graph[b][a]=w
    E(1,2,4); E(1,3,2); E(2,3,1); E(2,4,7);
    E(3,5,3); E(5,4,2); E(4,6,1); E(5,7,5);
    E(6,7,3); E(6,8,6); E(7,9,4); E(8,9,2);
    E(9,10,3); E(10,11,4); E(11,12,2); E(8,12,8);
#undef E
}

void printGraph(int graph[MAX_NODES][MAX_NODES], int node_count) {
    for (int i=1;i<=node_count;i++) {
        for (int j=1;j<=node_count;j++) {
            if (graph[i][j] >= INF) printf(" INF ");
            else printf("%4d ", graph[i][j]);
        }
        printf("\n");
    }
}

void dijkstra(int graph[MAX_NODES][MAX_NODES], int node_count, int src, int dist[], int parent[]) {
    int vis[MAX_NODES]={0};

    for (int i=1;i<=node_count;i++)
        dist[i]=INF, parent[i]=-1;

    if (src < 1 || src > node_count) return;
    dist[src]=0;

    for (int k=1;k<=node_count;k++) {

        int u=-1, best=INF;
        for (int i=1;i<=node_count;i++)
            if (!vis[i] && dist[i] < best)
                best = dist[i], u=i;

        if (u==-1) break;
        vis[u]=1;

        for (int v=1;v<=node_count;v++) {
            if (graph[u][v] < INF && dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v];
                parent[v] = u;
            }
        }
    }
}

void reconstructPath(int parent[], int src, int dest, int path[], int *len) {
    int stack[MAX_NODES], top=0;
    int cur = dest;

    *len=0;

    while (cur != -1 && cur != src) {
        stack[top++] = cur;
        cur = parent[cur];
    }

    if (cur == -1) { *len=0; return; }
    stack[top++] = src;

    for (int i=top-1;i>=0;i--)
        path[(*len)++] = stack[i];
}


// --------------------------------------------
// FIND NEAREST DRIVER
// --------------------------------------------

Driver* findNearestAvailableDriver(
        DriverNode* head,
        int node_count,
        int graph[MAX_NODES][MAX_NODES],
        int pickup,
        int *out_dist,
        int parent_out[])
{
    int dist[MAX_NODES], parent[MAX_NODES];
    dijkstra(graph,node_count,pickup,dist,parent);

    DriverNode* best=NULL;
    int bestd=INF;

    for (DriverNode* c=head; c; c=c->next) {
        if (c->driver_data.status == AVAILABLE) {
            if (c->driver_data.location < 1 || c->driver_data.location > node_count) continue;
            int d = dist[c->driver_data.location];
            if (d < bestd) {
                bestd = d;
                best = c;
            }
        }
    }

    if (best) {
        *out_dist = bestd;
        for (int i=0;i<=node_count;i++) parent_out[i]=parent[i];
        return &best->driver_data;
    }

    return NULL;
}


// --------------------------------------------
// RIDE SIMULATION
// --------------------------------------------

void simulateMovementAndComplete(
        Driver* driver,
        RideRequest *ride,
        int node_count,
        int graph[MAX_NODES][MAX_NODES])
{
    printf("\n[SIM] Driver %s starting movement...\n", driver->name);

    int dist[MAX_NODES], parent[MAX_NODES];

    /* compute shortest paths from pickup once */
    dijkstra(graph,node_count,ride->pickup,dist,parent);

    int path2[MAX_NODES], len2=0;
    reconstructPath(parent, ride->pickup, ride->drop, path2, &len2);

    if (len2 == 0) {
        printf("[ERROR] No route from pickup %d to drop %d.\n", ride->pickup, ride->drop);
        driver->status = AVAILABLE;
        return;
    }

    printf("[SIM] Path pickup → drop:\n");
    for (int i=0;i<len2;i++) printf(" -> %d\n", path2[i]);

    int travel = dist[ride->drop];
    if (travel >= INF) {
        printf("[ERROR] Drop unreachable.\n");
        driver->status = AVAILABLE;
        return;
    }

    double fare = travel * RATE_PER_UNIT;

    driver->earnings += fare;
    driver->location = ride->drop;
    driver->status = AVAILABLE;

    printf("\n[SUCCESS] Ride complete. Distance:%d Fare:%.2f\n", travel, fare);
}


// --------------------------------------------
// RIDE REQUEST SYSTEM
// --------------------------------------------

void createRideRequest(Queue* q) {
    if (!q) return;
    RideRequest r;
    char b[50];

    printf("Passenger name: ");
    if (!fgets(r.passengerName, sizeof(r.passengerName), stdin)) return;
    r.passengerName[strcspn(r.passengerName,"\n")] = 0;

    printf("Pickup (1-%d): ", NODE_COUNT);
    if (!fgets(b, sizeof(b), stdin)) return; r.pickup = atoi(b);

    printf("Drop (1-%d): ", NODE_COUNT);
    if (!fgets(b, sizeof(b), stdin)) return; r.drop = atoi(b);

    if (r.pickup < 1 || r.pickup > NODE_COUNT || r.drop < 1 || r.drop > NODE_COUNT) {
        printf("[ERROR] Pickup and Drop must be between 1 and %d. Request cancelled.\n", NODE_COUNT);
        return;
    }

    r.id = rand()%90000 + 10000;
    r.otp = rand()%9000 + 1000;

    enqueue(q,r);
    printf("[INFO] Ride queued. (DEBUG OTP: %d)\n", r.otp);
}

void viewQueuedRides(Queue* q) {
    if (!q || !q->front) { printf("No rides.\n"); return; }

    for (RideRequestNode* c=q->front; c; c=c->next) {
        printf("ID:%d | %s | OTP:%d | P:%d D:%d\n",
            c->request_data.id,
            c->request_data.passengerName,
            c->request_data.otp,
            c->request_data.pickup,
            c->request_data.drop);
    }
}

void assignDriverToRide(Queue* q, DriverNode* head) {
    if (!q || !q->front) {
        printf("[INFO] No rides.\n");
        return;
    }

    static int graph[MAX_NODES][MAX_NODES];
    int node_count;
    initGraph(graph, &node_count);

    RideRequest r = q->front->request_data;

    if (r.pickup < 1 || r.pickup > node_count || r.drop < 1 || r.drop > node_count) {
        printf("[ERROR] Ride pickup/drop out of bounds.\n");
        return;
    }

    int dval;
    int parent_out[MAX_NODES];

    Driver* d = findNearestAvailableDriver(head,node_count,graph,r.pickup,&dval,parent_out);

    if (!d) {
        printf("[INFO] No available drivers.\n");
        return;
    }

    printf("[MATCH] Nearest driver: %s at %d\n", d->name, d->location);

    char b[20];
    printf("Enter OTP: ");
    if (!fgets(b, sizeof(b), stdin)) return;

    if (atoi(b) != r.otp) {
        printf("[ERROR] Wrong OTP.\n");
        return;
    }

    d->status = ON_RIDE;

    simulateMovementAndComplete(d,&r,node_count,graph);

    RideRequest* rm = dequeue(q);
    if (rm) free(rm);
}

void completeRide(DriverNode* head) {
    printf("[INFO] Manual complete not required.\n");
}


// --------------------------------------------
// MEMORY CLEANUP
// --------------------------------------------

void freeDriverList(DriverNode** headRef) {
    DriverNode *c=*headRef, *n;
    while (c) {
        n=c->next;
        free(c);
        c=n;
    }
    *headRef=NULL;
}

void freeRideQueue(Queue* q) {
    if (!q) return;
    RideRequestNode *c=q->front, *n;
    while (c) {
        n=c->next;
        free(c);
        c=n;
    }
    free(q);
}


// --------------------------------------------
// MENU + MAIN
// --------------------------------------------

void displayMainMenu() {
    printf("\n---- MAIN MENU ----\n");
    printf("1. Add Driver\n");
    printf("2. View All Drivers\n");
    printf("3. Update Driver Status\n");
    printf("4. Remove Driver\n");
    printf("5. Create Ride Request\n");
    printf("6. View Ride Queue\n");
    printf("7. Assign Driver (Graph + OTP)\n");
    printf("9. Show Graph\n");
    printf("0. Save & Exit\n");
}

int main() {
    srand((unsigned)time(NULL));

    DriverNode* driver_list = NULL;
    Queue* ride_q = createQueue();

    loadDriversFromFile(&driver_list);

    static int graph[MAX_NODES][MAX_NODES];
    int node_count;
    initGraph(graph,&node_count);

    int ch;
    char b[20];

    while (1) {
        displayMainMenu();
        printf("Choice: ");
        if (!fgets(b, sizeof(b), stdin)) break;
        ch = atoi(b);

        switch(ch) {
            case 1: addDriver(&driver_list); break;
            case 2: viewAllDrivers(driver_list); break;
            case 3: updateDriverStatus(driver_list); break;
            case 4: removeDriver(&driver_list); break;
            case 5: createRideRequest(ride_q); break;
            case 6: viewQueuedRides(ride_q); break;
            case 7: assignDriverToRide(ride_q, driver_list); break;
            case 9: printGraph(graph,node_count); break;

            case 0:
                saveDriversToFile(driver_list);
                freeDriverList(&driver_list);
                freeRideQueue(ride_q);
                printf("[EXIT] Saved & Closed.\n");
                return 0;

            default:
                printf("[ERROR] Invalid.\n");
        }
    }

    /* cleanup on EOF */
    saveDriversToFile(driver_list);
    freeDriverList(&driver_list);
    if (ride_q) freeRideQueue(ride_q);
    return 0;
}
