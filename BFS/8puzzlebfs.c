#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define blank '0' // defining a blank tile

unsigned int nodesExpanded;  //number of expanded nodes
unsigned int nodesGenerated; //number of generated nodes
unsigned int solutionLength; //number of moves in solution
double runtime;              //elapsed time (in milliseconds)

typedef struct ListNode ListNode;
typedef struct NodeList NodeList;
typedef struct Node Node;

typedef enum moves
{
	UP, DOWN, LEFT, RIGHT, NA // the moves applicable to blank tile
} moves;

typedef struct State
{
	moves action;
	char gameboard[3][3]; // The game board is 3 x 3
} State;

struct ListNode
{
    Node *currNode;
    struct ListNode *prevNode; //the node before `this` instance
    struct ListNode *nextNode; //the next node in the linked list
};

struct NodeList
{
    unsigned int nodeCount;    //the number of nodes in the list
    ListNode *head;            //pointer to the first node in the list
    ListNode *tail;            //pointer to the last node in the list
};

struct Node
{
    unsigned int depth; //depth of the node from the root.
    State *state;       //state designated to a node
    Node *parent;       //parent node
    NodeList *children; //list of child nodes
};

Node* createNode(unsigned int d, State *s, Node *p)
{
	Node *newNode = malloc(sizeof(Node));

	if(newNode)
	{
		newNode->depth = d;
		newNode->state = s;
		newNode->parent = p;
		newNode->children = NULL;

		++nodesGenerated;
	}

	return newNode;
}

void destroyTree(Node *node)
{
    if(node->children == NULL)
    {
        free(node->state);
        free(node);
        return;
    }

    ListNode *listNode = node->children->head;
    ListNode *nextNode;

    while(listNode)
    {
        nextNode = listNode->nextNode;
        destroyTree(listNode->currNode);
        listNode = nextNode;
    }

    //free(node->state);
    free(node->children);
    free(node);
}

State* createState(State *state, moves move)
{
	State *newState = malloc(sizeof(State));

	char i, j;
	char row, column;

	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 3; j++)
		{
			if(state->gameboard[i][j] == blank) // getting coordinates of the blank tile
			{
				row = i;
				column = j;
			}

			newState->gameboard[i][j] = state->gameboard[i][j];
		}
	}

	if(move == UP && (row - 1) >= 0) // MOVING UP. min row is 0, if lower than 0 then it's not applicable.
	{
		char temp = newState->gameboard[row - 1][column];
		newState->gameboard[row - 1][column] = blank;
		newState->gameboard[row][column] = temp;
		newState->action = UP;
		return newState;
	}
	else if(move == DOWN && (row + 1) < 3) // MOVING DOWN. max row is 2, if higher than 2 then it's not applicable.
	{
		char temp = newState->gameboard[row + 1][column];
		newState->gameboard[row + 1][column] = blank;
		newState->gameboard[row][column] = temp;
		newState->action = DOWN;
		return newState;
	}
	else if(move == LEFT && (column - 1) >= 0) // MOVING LEFT. min column is 0, if lower then it's not applicable.
	{
		char temp = newState->gameboard[row][column - 1];
		newState->gameboard[row][column - 1] = blank;
		newState->gameboard[row][column] = temp;
		newState->action = LEFT;
		return newState;
	}
	else if(move == RIGHT && (column + 1) < 3) // MOVING RIGHT. max column is 2, if lower then it's not applicable.
	{
		char temp = newState->gameboard[row][column + 1];
		newState->gameboard[row][column + 1] = blank;
		newState->gameboard[row][column] = temp;
		newState->action = RIGHT;
		return newState;
	}

	free(newState);
	return NULL;
}

void destroyState(State **state)
{
	free(*state);
	state = NULL;
}

typedef struct SolutionPath
{
	moves action;
	struct SolutionPath *next;
} solutionPath;

void clearSolution(solutionPath **list)
{
    solutionPath *next;

    while(*list)
    {
        next = (*list)->next;
        free(*list);
        *list = next;
    }

    *list = NULL;
}

char pushNode(Node *node, NodeList** const list)
{
    if(!node)
    {
        return 0;
    }

    ListNode *doublyNode = malloc(sizeof(ListNode));
    if(!doublyNode)
    {
        return 0;
    }

    doublyNode->currNode = node;

    if(*list && !(*list)->nodeCount)
    {
        (*list)->head = doublyNode;
        (*list)->tail = doublyNode;
        doublyNode->nextNode = NULL;
        doublyNode->prevNode = NULL;
        ++(*list)->nodeCount;
        return 1;
    }

    if(*list == NULL)
    {
        *list = malloc(sizeof(NodeList));

        if(*list == NULL)
        {
            return 0;
        }

        (*list)->nodeCount = 0;
        (*list)->head = NULL;
        (*list)->tail = doublyNode;
    }
    else
    {
        (*list)->head->prevNode = doublyNode;
    }

    doublyNode->nextNode = (*list)->head;
    doublyNode->prevNode = NULL;
    (*list)->head = doublyNode;

    ++(*list)->nodeCount;

    return 1;
}

Node* popNode(NodeList** const list)
{
    if(!*list || (*list)->nodeCount == 0)
    {
        return NULL;
    }

    Node *popped = (*list)->tail->currNode;
    ListNode *prevNode = (*list)->tail->prevNode;

    //free the list node pointing to node to be popped
    free((*list)->tail);

    if((*list)->nodeCount == 1)
    {
        (*list)->head = NULL;
    }
	else
	{
		prevNode->nextNode = NULL;
	}

    (*list)->tail = prevNode;
    --(*list)->nodeCount;
    return popped;
}

void pushList(NodeList **toAppend, NodeList *list)
{
    //if either of the list is NULL, the head of the list to be appended is NULL,
    //or the list points to the same starting node
    if(!*toAppend || !list || !(*toAppend)->head || (*toAppend)->head == list->head)
    {
        return;
    }

    //if the list to append to has currently no element
    if(!list->nodeCount)
    {
        list->head = (*toAppend)->head;
        list->tail = (*toAppend)->tail;
    }
    else
    {
        //connect the lists
        (*toAppend)->tail->nextNode = list->head;
        list->head->prevNode = (*toAppend)->tail;
		list->head = (*toAppend)->head;
    }

    //update list information
    list->nodeCount += (*toAppend)->nodeCount;

    free(*toAppend);
    *toAppend = NULL;
}

char checkGoal(State const *checkState, State const *goalState)
{
	char i, j;

	for(i = 0; i < 3; i++)
	{
		for(j = 0; j < 3; j++)
		{
			if(checkState->gameboard[i][j] != goalState->gameboard[i][j]) // testing each tile so that it matched the goal state.
			{
				return 0; // if any of them not match one another, it return false.
			}
		}
	}

	return 1; // else, return true.
}

void inputState(State * const state)
{
	state->action = NA;
	char row, column;
	int temp;
	
	for(row = 0; row < 3; row++)
	{
		for(column = 0; column < 3; column++)
		{
			printf("gameboard[%i][%i] : ", row, column);
			scanf("%i", &temp);
			state->gameboard[row][column] = temp + '0';
		}
	}
	printf("\n");
}

void inputGoalState(State * const state)
{
	state->action = NA;
	state->gameboard[0][0] = '1';
	state->gameboard[0][1] = '2';
	state->gameboard[0][2] = '3';
	state->gameboard[1][0] = '8';
	state->gameboard[1][1] = '0';
	state->gameboard[1][2] = '4';
	state->gameboard[2][0] = '7';
	state->gameboard[2][1] = '6';
	state->gameboard[2][2] = '5';

	printf("\n");
}

void printBoard(char const gameboard[3][3])
{
	char row, column;

	for(row = 0; row < 3; row++)
	{
		printf("+---+---+---+\n");
		for(column = 0; column < 3; column++)
		{
			if(gameboard[row][column] == '0')
			{
				printf("|   ");
			}
			else
			{
				printf("| %c ", gameboard[row][column]);
			}
		}
		printf("|\n");
	}
	printf("+---+---+---+\n");
}

void printSolution(solutionPath *path)
{
	if(!path)
	{
		printf("No solution found.\n");
		return;
	}

	if(!path->next)
	{
		printf("The initial state is the goal state.\n");
		return;
	}

	printf("Solution : (Moves are based on the blank tile moves)\n");

	char *move[4] = { "UP", "DOWN", "LEFT", "RIGHT" };
    int counter = 1;

    //will be skipping the first node since it represents the initial state with no action
    for(path = path->next; path; path = path->next, ++counter)
    {
        printf("%d. Move %s\n", counter, move[path->action]);
    }

    printf(
        "DETAILS:\n"
        " - Solution length : %d\n"
        " - Nodes expanded  : %d\n"
        " - Nodes generated : %d\n"
        " - Runtime         : %lf milliseconds\n"
        " - Memory used     : %d bytes\n", //only counting allocated `Node`s
        solutionLength, nodesExpanded, nodesGenerated, runtime, nodesGenerated * sizeof(Node));
}

NodeList* getChildren(Node *parent, State *goalState)
{
    NodeList *childrenPtr = NULL;
    State *testState = NULL;
    Node *child = NULL;

    //attempt to create states for each moves, and add to the list of children if true
    if(parent->state->action != DOWN && (testState = createState(parent->state, UP))) {
        child = createNode(parent->depth + 1, testState, parent);
        pushNode(child, &parent->children);
        pushNode(child, &childrenPtr);
    }
    if(parent->state->action != UP && (testState = createState(parent->state, DOWN))) {
        child = createNode(parent->depth + 1, testState, parent);
        pushNode(child, &parent->children);
        pushNode(child, &childrenPtr);
    }
    if(parent->state->action != RIGHT && (testState = createState(parent->state, LEFT))) {
        child = createNode(parent->depth + 1, testState, parent);
        pushNode(child, &parent->children);
        pushNode(child, &childrenPtr);
    }
    if(parent->state->action != LEFT && (testState = createState(parent->state, RIGHT))) {
        child = createNode(parent->depth + 1, testState, parent);
        pushNode(child, &parent->children);
        pushNode(child, &childrenPtr);
    }

    return childrenPtr;
}

solutionPath* BFS_search(State *init, State *goalState);

int main()
{
	nodesExpanded = 0;
    nodesGenerated = 0;
    solutionLength = 0;
    runtime = 0;

	printf("Hello! Welcome to the 8-puzzle solver!\n");

	State init;
	State goalState;

	solutionPath *bfs;

	printf("Please input the initial state:\n");
    inputState(&init);

    inputGoalState(&goalState);

    printf("Here's the initial board state:\n");
    printBoard(init.gameboard);

    printf("Here's the board's goal state:\n");
    printBoard(goalState.gameboard);
    
    printf("\n------------------------- USING BFS ALGORITHM --------------------------\n");
    bfs = BFS_search(&init, &goalState);
    printSolution(bfs);

    //free resources
    //clearSolution(&bfs);
}

solutionPath* BFS_search(State *init, State *goalState)
{
	NodeList *queue = NULL;
    NodeList *children = NULL;
    Node *node = NULL;

    clock_t start = clock();

    pushNode(createNode(0, init, NULL), &queue);
    Node *root = queue->head->currNode;

    while(queue->nodeCount > 0)
    {
        //pop the last node (tail) of the queue
        node = popNode(&queue);

        //if the state of the node is the goal state
        if(checkGoal(node->state, goalState))
        {
            break;
        }

        //else, expand the node and update the expanded-nodes counter
        children = getChildren(node, goalState);

        ++nodesExpanded;

        //add the node's children to the queue
        pushList(&children, queue);
    }

    runtime = (double)(clock() - start) / CLOCKS_PER_SEC;

    //get solution path in order from the root, if it exists
    solutionPath *pathHead = NULL;
    solutionPath *newPathNode = NULL;

    while(node)
    {
        newPathNode = malloc(sizeof(solutionPath));
        newPathNode->action = node->state->action;
        newPathNode->next = pathHead;
        pathHead = newPathNode;

        //update the solution length and move on the next node
        ++solutionLength;
        node = node->parent;
    }

    --solutionLength; //uncount the root node

    //deallocate the generated tree
    destroyTree(root);

    return pathHead;
}