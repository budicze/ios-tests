#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#define H 'H'
#define O 'O'

typedef enum {
	prestarted, started, waiting, ready, begin, bonded, finished, invalid,
} State;

typedef struct {
	char atomType;
	unsigned actionID;
	unsigned atomID;
	State state;
} Row;

State conversionTable[] = {100, prestarted, started, started, 101, begin, bonded, 100};

char *conv[] = {"prestarted", "started", "waiting", "ready", "begin", "bonded", "finished", "invalid"};

State convertState(const char *str){
	if(strcmp(str, "started") == 0) return started;
	else if(strcmp(str, "waiting") == 0) return waiting;
	else if(strcmp(str, "ready") == 0) return ready;
	else if(strcmp(str, "begin") == 0) return begin;
	else if(strcmp(str, "bonded") == 0) return bonded;
	else if(strcmp(str, "finished") == 0) return finished;

	return invalid;
}

void error(unsigned row, FILE *f, const char *fmt, ...){

	fprintf(stderr, "Row %u: ", row);
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");

	fclose(f);

	exit(1);
}

int main(int argc, char *argv[]){

	if(argc < 2){
		fprintf(stderr, "1st argument should be number of created molecules!\n");
		return 1;
	}

	unsigned molecules = atoi(argv[1]);

	State *h = calloc(2 * molecules, sizeof(State));
	State *o = calloc(molecules, sizeof(State));

	FILE *f = fopen("h2o.out", "r");

	if(f == NULL){
		fprintf(stderr, "No h2o.out file found!");
		return 1;
	}

	char buffer[256];

	unsigned i = 1;
	unsigned hBonding = 0;
	unsigned oBonding = 0;
	bool isReady = false;
	bool waitingToBeBonded = false;
	unsigned moleculesDone = 0;

	while(fgets(buffer, 256, f) != NULL){
		Row r;

		char action[256];
		int ret = sscanf(buffer, "%u : %c %u : %s", &r.actionID, &r.atomType, &r.atomID, action);

		if(ret != 4){
			error(i, f, "Row cannot be converted!");
		}

		r.state = convertState(action);
		if(r.state == invalid){
			error(i, f, "Invalid state: %s", action);
		}

		if(r.actionID != i){
			error(i, f, "Not matching row, number \"%u\" found!", r.actionID);
		}

		State *a = (r.atomType == H) ? &h[r.atomID - 1] : &o[r.atomID - 1];
		State expectedState = conversionTable[r.state];

		if(expectedState == 101) {
			if(*a != ready && *a != waiting){
				error(i, f, "Atom %u cannot convert from state %s to state %s!", r.atomID, conv[*a], conv[r.state]);
			}
		} else if(*a != expectedState){
			error(i, f, "Atom %u cannot convert from state %s to state %s!", r.atomID, conv[*a], conv[r.state]);
		}

		switch(r.state){
			case waiting:
				if((hBonding + oBonding) > 0 || isReady){
					error(i, f, "Atom %u cannot move to waiting while molecule is being bonded!", r.atomID);
				}
				break;
			case ready:
				if((hBonding + oBonding) > 0 || isReady){
					error(i, f, "Atom %u cannot be ready if other atom is ready!", r.atomID);
				}
				isReady = true;
				break;
			case begin:
				if(r.atomType == H && hBonding > 1){
					error(i, f, "Atom %u cannot join bonding if there are 2 hydrogen already bonding!", r.atomID);
				} else if(r.atomType == O && oBonding > 0){
					error(i, f, "Atom %u cannot join bonding if there is an oxygen already bonding!", r.atomID);
				}

				if(!isReady){
					error(i, f, "Atom %u cannot join bonding, no atom has said 'ready'!", r.atomID);
				}

				(r.atomType == H) ? hBonding++ : oBonding++;

				if(hBonding == 2 && oBonding == 1) {
					waitingToBeBonded = true;
				}
				break;
			case bonded:
				if(!waitingToBeBonded){
					error(i, f, "Atom %u cannot bond just yet!", r.atomID);
				}

				(r.atomType == H) ? hBonding-- : oBonding--;

				if(hBonding == 0 && oBonding == 0){
					waitingToBeBonded = false;
					moleculesDone++;
					isReady = false;
				}
				break;
			case finished:
				if(moleculesDone < molecules){
					error(i, f, "Atom %u cannot finish before all molecules are done! Molecules done so far: %u", r.atomID, moleculesDone);
				}
				break;

			default:
			break;
		}

		*a = r.state;

		i++;
	}

	for(unsigned j = 0; j < molecules; j++){
		if(h[2 * j] != finished){
			error(i, f, "Hydrogen atom #%d is not finished!", 2 * j + 1);
		}
		if(h[2 * j + 1] != finished){
			error(i, f, "Hydrogen atom #%d is not finished!", 2 * j + 1 + 1);
		}
		if(o[j] != finished){
			error(i, f, "Oxygen atom #%d is not finished!", j + 1);
		}
	}

	fclose(f);

	return 0;
}
