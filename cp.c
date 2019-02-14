#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "LinkedList.h"
#include "Channel.h"
#include "Relation.h"

#define MAX 100
#define MAX_UTIL 0.9

int n = 0;
int **constraint = NULL;
int *vars = NULL;
struct LinkedList **domains = NULL;

int *solution = NULL;
int n_sol_elements = 0;

void clean_up();

int** read_topo_from_file(char* filename);
int* read_channels_from_file(char* filename);
void print_topo(int n, int** constraint);
void print_channel_utilization(int n, struct LinkedList **domains);
void print_solution();
struct Channel get_current_channel(int index);

/* for constraint propagation */
void node_consistent();
int revise(int v1, int v2);
int arc_consistent();
/*****************************/

/* for backtracking */
int consistent(int var, int value);
int search_bt(int level);

int main(){
	srand((time(0)));
	int i;
	constraint = read_topo_from_file("topo.txt");
	
	vars = read_channels_from_file("channels.txt");

	solution = malloc(n*sizeof(int));

	domains = malloc(n*sizeof(struct LinkedList*));
	for (i = 0; i < n; i++){
		domains[i] = newLinkedList();
		//vars[i] = malloc(sizeof(struct Channel));
		/* generate channel utilization randomly */
		/* total_util: [0.3; MAX_UTIL]
		 * env_util: [0.3; total_util] 
		 * 0.3 is the minimum environment ultilization*/
		float a = MAX_UTIL - 0.3;
		int j, c = 1;
		for (j = 0; j < 3; j++){
			struct Channel channel;
			channel.chan_no = c;
			channel.total_util = ((float)rand()/(float)(RAND_MAX)) * a + 0.3;
			channel.env_util = ((float)rand()/(float)(RAND_MAX)) * (channel.total_util - 0.3) + 0.3;
			c += 5;
			struct Node *node = newNode((void*)&channel, CHANNEL_TYPE);
			push(domains[i], node);
		}
	}

	//n = 3;
	//constraint = malloc(n*sizeof(int*));
	//for (i = 0; i < n; i ++){
	//	constraint[i] = malloc(n*sizeof(int));
	//	int j;
	//	for (j = 0; j < n; j++){
	//		if (i == j){
	//			constraint[i][j] = 0;
	//		} else {
	//			constraint[i][j] = 1;
	//		}
	//	}
	//}

	//print_topo(n, constraint);

	//vars = malloc(n*sizeof(int));
	//vars[0] = 1;
	//vars[1] = 6;
	//vars[2] = 11;

	//domains = malloc(n*sizeof(struct LinkedList*));
	//for(i = 0; i < n; i ++){
	//	domains[i] = newLinkedList();
	//}

	//struct Node *node = NULL;
	//
	//struct Channel chan;
	//chan.chan_no = 1;
	//chan.env_util = 1.0;
	//chan.total_util = 1.0;
	//node = newNode(&chan, CHANNEL_TYPE);
	//push(domains[0], node);

	//chan.chan_no = 6;
	//chan.env_util = 6.0;
	//chan.total_util = 6.0;
	//node = newNode(&chan, CHANNEL_TYPE);
	//push(domains[0], node);
	//
	//chan.chan_no = 1;
	//chan.env_util = 1.0;
	//chan.total_util = 1.0;
	//node = newNode(&chan, CHANNEL_TYPE);
	//push(domains[1], node);

	//chan.chan_no = 6;
	//chan.env_util = 6.0;
	//chan.total_util = 6.0;
	//node = newNode(&chan, CHANNEL_TYPE);
	//push(domains[2], node);

	//print_topo(n, constraint);
	printf("channel utilization before performing node_consistent\n");
	print_channel_utilization(n, domains);
	node_consistent();
	printf("channel utilization after performing node_consistent\n");
	print_channel_utilization(n, domains);
	arc_consistent();
	printf("channel utilization after performing arc_consistent\n");
	print_channel_utilization(n, domains);
	if (search_bt(0)){
		printf("solution:\n");
		print_solution();
	} else {
		printf("no solution\n");
	}
	return 0;
}

int** read_topo_from_file(char* filename){
	FILE *file = fopen(filename, "r");
	fscanf(file, "%d", &n);
	int **constraint = NULL;
	int i;
	constraint = malloc(n*sizeof(int*));
	for (i = 0; i < n; i++){
		constraint[i] = malloc(n*sizeof(int));
	}

	for (i = 0; i < n; i++){
		int j;
		for (j = 0; j < n; j++){
			fscanf(file, "%d", &constraint[i][j]);
		}
	}
	fclose(file);

	return constraint;
}

int* read_channels_from_file(char *filename){
	FILE *file = fopen(filename, "r");
	int *channel = malloc(n*sizeof(int));
	int i;
	for (i = 0; i < n; i++){
		fscanf(file, "%d", &channel[i]);
	}
	fclose(file);
	return channel;
}

void print_topo(int n, int** constraint){
	int i;
	for (i = 0; i < n; i++){
		int j;
		for (j = 0; j < n; j ++){
			printf("%-3d ", constraint[i][j]);
		}
		printf("\n");
	}
}

void print_channel_utilization(int n, struct LinkedList **domains){
	int i;
	for (i = 0; i < n; i++){
		printf("%3d: ", i + 1);
		struct Node *it = domains[i]->head;
		for (; it != NULL; it = it->next){
			struct Channel channel = *(struct Channel*)it->data;
			printf("(%f, %f) ", channel.total_util, channel.env_util);
		}
		printf("\n");
	}
}

struct Channel get_current_channel(int index){
	int chan_no = vars[index];
	struct Node *it = domains[index]->head;
	for (; it != NULL; it = it->next){
		struct Channel *chan = (struct Channel*)it->data;
		if (chan->chan_no == chan_no)
			return *chan;
	}
	struct Channel new_chan;
	new_chan.chan_no = 0;
	return new_chan;
}

void node_consistent(){
	printf("performing node_consistent\n");
	int i;
	for (i = 0; i < n; i ++){
		struct Channel cur_chan = get_current_channel(i);
		float current_bss_util = cur_chan.total_util - cur_chan.env_util;
		printf("%3d: %d, %f, %f, %f\n", i + 1, cur_chan.chan_no, cur_chan.total_util, cur_chan.env_util, current_bss_util);
		struct Node *it = domains[i]->head;
		int remove_head = 0;
		for (; it != NULL; it = it->next){
			struct Channel *chan = (struct Channel*)it->data;
			if (current_bss_util + chan->env_util >= MAX_UTIL){
				/* remove this channel from the domain */
				if (it == domains[i]->head)
					remove_head = 1;
				else {
					it = it->prev;
					removeNode(domains[i], it->next);
				}
			}
		}
		if (remove_head)
			pop(domains[i]);
	}
}

int revise(int v1, int v2){
	//printf("start revising\n");
	int deleted = 0;
	int found = 0;
	int remove_head = 0;
	struct LinkedList *d1 = domains[v1];
	struct LinkedList *d2 = domains[v2];
	struct Node *it1 = d1->head;
	//printf("revise preparation done\n");
	for (; it1 != NULL; it1 = it1->next){
		found = 0;
		struct Channel *chan1 = (struct Channel*)it1->data;
		struct Node *it2 = d2->head;
		for (; it2 != NULL; it2 = it2->next){
			struct Channel *chan2 = (struct Channel*)it2->data;
			if (chan1->chan_no != chan2->chan_no){
				found = 1;
				break;
			}
		}
		//printf("finding done\n");
		if (!found){
			if (it1 == d1->head)
				remove_head = 1;
			else {
				it1 = it1->prev;
				removeNode(d1, it1->next);
			}
			deleted = 1;
		}
		//printf("removing channel %d done\n", chan1->chan_no);
	}
	if (remove_head)
		pop(d1);
	
	return deleted;
}

int arc_consistent(){
	/* initialization */
	struct LinkedList *queue = newLinkedList();
	//printf("created revise queue\n");
	int i, j;
	struct Relation re;
	for (i = 0; i < n; i++){
		for (j = 0; j < n; j++){
			if (constraint[i][j]){
				//printf("push (%d, %d)\n", i, j);
				re.i = i;
				re.j = j;
				struct Node *node = newNode(&re, RELATION_TYPE);
				push(queue, node);
			}
		}
	}
	//printf("init revise queue done\n");

	/* examination and propagation */
	while (queue->n_nodes){
		struct Node *node = pop(queue);
		//printf("pop queue to examine\n");
		struct Relation *r = (struct Relation*)(node->data);
		//printf("pop (%d, %d)\n", r->i, r->j);
		if (revise(r->i, r->j)){
			if (empty(domains[r->i])){
				return 0;
			} else {
				int k;
				for (k = 0; k < n; k++){
					if (k != r->j && constraint[k][r->i]){
						re.i = k;
						re.j = r->i;
						struct Node *node = newNode(&re, RELATION_TYPE);
						push(queue, node);
					}
				}
			}
		}
	}

	return 1;
}

void clean_up(){
	int i;
	for (i = 0; i < n; i ++){
		free(constraint[i]);
		destructLinkedList(domains[i]);
	}
	free(constraint);
	free(domains);
	free(vars);
	free(solution);
}

int consistent(int var, int value){
	/* check against the past variables */
	int i;
	for (i = 0; i < n_sol_elements; i ++){
		if (constraint[i][var] && value == solution[i])
			return 0;
	}
	return 1;
}

int search_bt(int level){
	/* select the var at index level */
	struct Node *it = domains[level]->head;
	for (; it != NULL; it = it->next){
		struct Channel *chan = (struct Channel*)(it->data);
		if (consistent(level, chan->chan_no)){
			solution[level] = chan->chan_no;
			n_sol_elements ++;
			if (level == n - 1){
				/* found a solution */
				return 1;
			} else {
				if (search_bt(level + 1))
					return 1;
			}
		} else
			continue;
		n_sol_elements --;
	}
	/* no solution found */
	return 0;
}

void print_solution(){
	int i;
	for (i = 0; i < n; i ++){
		printf("AP %3d: %d\n", i+1, solution[i]);
	}
}
