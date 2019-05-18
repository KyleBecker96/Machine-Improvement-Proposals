/***************************************************************************************************  
 * cs2123p4.c by Kyle Becker(cgc767)
 * Purpose:
 *      Runs a simulation that processes multiple widgets into a system containing
 *          two queues and two servers. Then calculates the average wait time for
 *          both those queues, and the average time the widgets were in the system.
 * Input:
 *      The standard input file reads in a widget's number, time units for steps needed
 *          to complete server, time the next widget arrives, and which server the widget enters
 * Results:
 *      Declares when each widget arrives, enters queue, leaves queue, seizes server, releases server,
 *          and exits the system.
 *      Calculates the average wait time for both those queues, 
 *          and the average time the widgets were in the system.
 * Notes:
 *      This program uses a Simulation that holds an ordered LinkedList containing
 *      events
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cs2123p4.h"
/************************* Prototype declarations *****************************/
void processCommandSwitches(int argc, char *argv[], Simulation sim);

Simulation newSimulation();
Server newServer(char szServerNm[]);
void freeQueue(Queue q);

void generateArrivals(Simulation sim);
void arrival(Simulation sim, Widget *widget);
void queueUp(Simulation sim, Queue queueTeller, Widget *widget);
void seize(Simulation sim, Queue queue, Server server);
void release(Simulation sim, Queue queueTeller, Server serverTeller, Widget *pWidget);
void leaveSystem(Simulation sim, Widget *pWidget);
/******************************************************************************/

FILE *pFile;

int main(int argc, char** argv) {

    Simulation sim = newSimulation();

    // get file names for command arg switches
    processCommandSwitches(argc, argv, sim);

    // call runSimulation
    runSimulation(sim);

    return (EXIT_SUCCESS);
}
/****************************** runSimulation **********************************
 *      void runSimulation(Simulation simulation);
 * 
 * Purpose:
 *      Prints out ordered Person info from eventList
 * 
 * Parameters:
 *      I Simulation simulation       simulation holds eventList with Person info
 * 
 * Notes:
 *      Calculates averages for queue times and system time
 * 
 * Returns:
 *      void
*******************************************************************************/
void runSimulation(Simulation sim)
{
    // declare variables
    double iAvgQTmSrvrM;
    double iAvgQTmSrvrW;
    double iAvgSysTm;

    // declare event
    Event event;

    // declare queues 
    Queue queueW = newQueue("W");
    Queue queueM = newQueue("M");

    // declare servers
    Server serverW = newServer("W");
    Server serverM = newServer("M");

    // call generateArrivals
    generateArrivals(sim);

    // format header if verbose or not
    if(sim->bVerbose == TRUE)
        printf("%s\t %s\t %s\n", "Time", "Widget", "Event");

    // loop through timed events
    while(removeLL(sim->eventList, &event))
    {
        // set sim->iClock to time of current timed event
        sim->iClock = event.iTime;

        // depending on iEventType, print, time, person, and event
        switch(event.iEventType)
        {
            case EVT_ARRIVAL:

                arrival(sim, &event.widget);

                if(event.widget.iWhichServer == 2)
                {
                    queueUp(sim, queueW, &event.widget);
                    seize(sim, queueW, serverW);
                }
                else
                {
                    queueUp(sim, queueM, &event.widget);
                    seize(sim, queueM, serverM);
                }
                break;

            case EVT_SERVERM_COMPLETE:
                release(sim, queueM, serverM, &event.widget);
                leaveSystem(sim, &event.widget);
                break;

            case EVT_SERVERW_COMPLETE:
                release(sim, queueW, serverW, &event.widget);
                leaveSystem(sim, &event.widget);
                break;

            default:
                errExit("Unknown event type: %d\n", event.iEventType);
        }
    }
    // print SIMULATION TERMINATES at end of sim
    printf("%d\t \t %s\n", event.iTime, "Simulation Complete for Alternative A");

    // Calculate average times and print
    iAvgQTmSrvrM = (double)queueM->lQueueWaitSum/queueM->lQueueWidgetTotalCount;
    iAvgQTmSrvrW = (double)queueW->lQueueWaitSum/queueW->lQueueWidgetTotalCount;
    iAvgSysTm = (double)sim->lSystemTimeSum/sim->lWidgetCount;

    printf("%38s %.1lf\n"  ,"Average Queue Time for Server M ="
                        ,iAvgQTmSrvrM);

    printf("%38s %.1lf\n " ,"Average Queue Time for Server W ="
                        ,iAvgQTmSrvrW);

    printf("%29s %.1lf\n"  , "Average Time in System = "
                        , iAvgSysTm);

    freeQueue(queueM);
    freeQueue(queueW);
    free(serverM);
    free(serverW);
    free(sim->eventList);
    free(sim);

}
/****************************** generateArrivals **************************************
 * void generateArrivals(Simulation sim)
 * 
 * Purpose:
 *      Read information from the input file and store information in a linked list
 * Parameters:
 *      Simulation sim
 * Notes:
 *      Inserts events into an ordered eventList inside of Sim
 * Return type:
 *      Void
 * ****************************************************************************/
void generateArrivals(Simulation sim)
{
    pFile = stdin;
    Widget widget;
    Event eventArrive;
    int iScanfCount;
    int iArrivalDelta;
    char szInputBuffer[MAX_LINE_SIZE+1];


    while(fgets(szInputBuffer, sizeof(szInputBuffer), pFile) != NULL)
    {
        if (szInputBuffer[0] == '\n')
            continue;

        iScanfCount = sscanf(szInputBuffer, "%ld %d %d %d %d"
                                , &widget.lWidgetNr
                                , &widget.iStep1tu
                                , &widget.iStep2tu
                                , &iArrivalDelta
                                , &widget.iWhichServer);

        if(iScanfCount != 5)
            errExit("Invalid input of event information");

        //set arrive event info
        eventArrive.iEventType = EVT_ARRIVAL;
        eventArrive.widget = widget;
        eventArrive.iTime = sim->iClock;

        // insert arrive into sim's linked list
        insertOrderedLL(sim->eventList, eventArrive);

        //increment clock so the next arrival is correct
        sim->iClock += iArrivalDelta;
    }
}
/******************************** arrival ***************************************
 * void arrival(Simulation sim, Widget *pWidget);
 * 
 * Purpose:
 *      sets the arrival time and prints verbose if verbose switch
 * 
 * Parameters:
 *      I   Simulation  sim
 *      I/O Widget      *pWidget
 * 
 * Returns:
 *      void
 * ****************************************************************************/
void arrival(Simulation sim, Widget *pWidget)
{
    //set arrival time
    pWidget->iArrivalTime = sim->iClock;

    // if verbose flag, print verbose
    if(sim->bVerbose == TRUE)
        printf("%d\t %ld\t %s\n", sim->iClock
                                , pWidget->lWidgetNr
                                , "Arrived");
}
/********************************* queueUp ************************************
 * void queueUp(Simulation sim, Queue queueTeller, Widget widget)
 * 
 * Purpose:
 *      Sets the EnterQTime and stores a QElement inside a specific Queue
 * Parameters:
 *      I   Simulation  sim
 *      I/O Queue       queueTeller
 *      I   Widget      *pWidget
 * Notes:
 *      Increments WidgetTotalCount by one after every insertion of new qElement
 * ****************************************************************************/
void queueUp(Simulation sim, Queue queueTeller, Widget *pWidget)
{
    //declare variables
    QElement qElement;

    //set the enterQTime
    qElement.iEnterQTime = sim->iClock;

    //populate widget struct
    qElement.widget = *pWidget;

    //insert widget in queue
    insertQ(queueTeller, qElement);

    //increment WidgetTotalCount for each widget entered in Q
    queueTeller->lQueueWidgetTotalCount++;

    if(sim->bVerbose == TRUE)
        printf("%d\t %ld\t Enter Queue %s\n", sim->iClock
                                            , qElement.widget.lWidgetNr
                                            , queueTeller->szQName);
}
/********************************** seize **************************************
 * void seize(Simulation sim, Queue queueTeller, Server serverTeller);
 * 
 * Purpose:
 *      Stores the widget from the queue into the server
 * Parameters:
 *      I/O Simulation  sim
 *      I/O Queue       queue
 *      I/O Server      server
 * Notes:
 *      Only runs if server isn't busy
 *      Uses removeQ to store QElement into Server
 *      When server completes, change event type and insert into linked list
 * ****************************************************************************/
void seize(Simulation sim, Queue queue, Server server)
{

    //Only run if server not marked busy
    if(server->bBusy == FALSE)
    {
        QElement qElement;
        Event event;
        int iWait;

        //mark the server as busy, remove widget from queue
        server->bBusy = TRUE;

        // if removeQ return True, take statistic on time & add to sum of queue
        removeQ(queue, &qElement);

        // find the time waited in queue by sutracting the time you entered queue by the time it is now
        iWait = sim->iClock - qElement.iEnterQTime;

        // to find sum of all queue wait times just add time waited to QueueWaitSum
        queue->lQueueWaitSum += iWait;

        //place widget removed from queue into server
        server->widget = qElement.widget;

        // Find the time the seize completed by adding the time it took to complete both steps    
        event.iTime = sim->iClock + server->widget.iStep1tu + server->widget.iStep2tu;

        //storing widget from server into an event's widget to set the event type
        event.widget = server->widget;

        if(server->widget.iWhichServer == 1)
        {
            event.iEventType = EVT_SERVERM_COMPLETE;

            if(sim->bVerbose == TRUE)
                printf("%d\t %ld\t Leave Queue M, waited %d\n"  , sim->iClock
                                                                , qElement.widget.lWidgetNr
                                                                , iWait);
        }
        else
        {
            event.iEventType = EVT_SERVERW_COMPLETE;

            if(sim->bVerbose == TRUE)
                printf("%d\t %ld\t Leave Queue W, waited %d\n"  ,sim->iClock
                                                                ,qElement.widget.lWidgetNr
                                                                ,iWait);
        }

        // store the event into linked list so switch in runSim can check the event type
        insertOrderedLL(sim->eventList, event);

        // if verbose flag is active
        if(sim->bVerbose == TRUE)
            printf("%d\t %ld\t Seized Server %s\n"  , sim->iClock
                                                    , server->widget.lWidgetNr
                                                    , server->szServerName);
    }

}
/******************************** release **************************************
 * void release(Simulation sim, Queue queueTeller, Server serverTeller, Widget *pWidget);
 * 
 * Purpose: Releases the widget from the server
 * 
 * Parameters:
 *      I/O   Simulation  sim
 *      I/O   Queue       queueTeller
 *      I/O   Server      serverTeller
 *      I     Widget      *pWidget
 * 
 * Notes:
 *      Prints verbose if verbose flag is switched on
 * Return:
 *      void
 * ****************************************************************************/
void release(Simulation sim, Queue queueTeller, Server serverTeller, Widget *pWidget)
{
    //set server to not busy
    serverTeller->bBusy = FALSE;

    // seize if not empty
    if(queueTeller->pHead != NULL)
        seize(sim, queueTeller, serverTeller);

    //if verbose switch active, print verbose
    if(sim->bVerbose == TRUE)
        printf("%d\t %ld\t Released Server %s\n",sim->iClock
                                                ,pWidget->lWidgetNr
                                                ,serverTeller->szServerName);


}
/******************************** leaveSystem **********************************
 * void leaveSystem(Simulation sim, Widget *pWidget)
 * 
 * Purpose: 
 *      Processes total time the widget was in the system
 * 
 * Parameters:  
 *      I/O Simulation sim
 *      I   Widget     *pWidget
 * 
 * Notes:
 *      Prints verbose if verbose flag is switched on
 * 
 * Returns:
 *      void      
 ******************************************************************************/
void leaveSystem(Simulation sim, Widget *pWidget)
{
    //Declare variables
    Widget widget;
    int iSystemSumTm;

    widget = *pWidget;

    // set system time     
    iSystemSumTm = sim->iClock - widget.iArrivalTime;

    if(sim->bVerbose == TRUE)
    {
        printf("%d\t %ld\t Exit System, in system %d\n" ,sim->iClock
                                                        ,widget.lWidgetNr
                                                        ,iSystemSumTm);
    }

    // increment processed widgets
    sim->lWidgetCount++;

    // increment sysTotalSum
    sim->lSystemTimeSum += iSystemSumTm;
}
/***************************** newSimulation ***********************************
        Simulation newSimulation();
 * 
 * Purpose:
 *      Malloc and intialize a Simulation structure
 * 
 * Notes:
 *      Sets iClock to 0
 *      Initialize LL eventList using newLinkedList
 * 
 * Returns data type:
 *      Simulation
*******************************************************************************/
Simulation newSimulation()
{
    // malloc and initialize Simulation struct
    Simulation simulation = (Simulation) malloc(sizeof(SimulationImp));
    if(simulation == NULL)
        errExit("Could not allocate Simulation");

    // set iClock to 0
    simulation->iClock = 0;

    //initialize eventList using newLinkedList
    simulation->eventList = newLinkedList();

    return simulation;
}
/****************************** newServer **************************************
 * Server newServer(char szServerNm[]);
 * 
 * Parameters:
 *      char szServerNm[]       Name of the server
 * Returns:
 *      Server s
*******************************************************************************/
Server newServer(char szServerNm[])
{
    Server s = (Server)malloc(sizeof(ServerImp));
    if (s == NULL)
        errExit("Could not allocate Server");

    s->bBusy = FALSE;
    strcpy(s->szServerName,szServerNm);
    return s;
}
/**************************** newLinkedList ************************************
        LinkedList newLinkedList();
Purpose:
    Creates new linked list and sets it to empty.

Notes:
    Uses LinkedList and LinkedListImp
 
Return value:
    LinkedList       pointer to LinkedListImp
 ******************************************************************************/
LinkedList newLinkedList()
{
    LinkedList list = (LinkedList) malloc(sizeof(LinkedListImp));
    if(list == NULL)
        errExit("Could not allocate LinkedList");

    list->pHead = NULL;
    return list;
}
/***************************** allocNodeLL *************************************
        NodeLL *allocNodeLL(LinkedList list, Event value);
Purpose:
    allocates memory for a new pointer to node

Parameters:
    I LinkedList list           Linked list
    I Event value               Event typedef struct variable

Notes:
    stores passed in value into node
    sets new node's next to NULL
 ******************************************************************************/
NodeLL *allocNodeLL(LinkedList list, Event value)
{
    //declare variables
    NodeLL *pNew;

    // malloc and initialize pNew in type NodeLL, error check
    pNew = (NodeLL *)malloc(sizeof(NodeLL));
    if(pNew == NULL)
        errExit("Could not allocate NodeLL");

    // store passed value into pNew->event
    pNew->event = value;

    // set pNew's next to NULL
    pNew->pNext = NULL;

    return pNew;
}
/****************************** searchLL ***************************************
        NodeLL *searchLL(LinkedList list, int match, NodeLL **ppPrecedes);
 * Purpose:
 *      iterates through linked list searching for matching value in a node
 * 
 * Parameters:
 *      I LinkedList list               Linked list
 *      I int match                     Value being searched for
 *      I/O NodeLL **ppPrecedes         address of pointer to preceding node
 * 
 * Notes:
 *      **ppPrecedes passed in as double pointer so it can be changed
 * 
 * Return value:
 *      NULL            If no matching value found in node
 *      NodeLL *        Returns NodeLL containing matching value
*******************************************************************************/
NodeLL *searchLL(LinkedList list, int match, NodeLL **ppPrecedes)
{
    //declare variables
    NodeLL *p;

    //NULL when list is empty, or need to insert at beginning
    *ppPrecedes = NULL;

    //traverse through list looking for where key belongs or end of list
    for(p = list->pHead; p != NULL; p = p->pNext)
    {
        if(match == p->event.iTime)
            return p;
        if(match < p->event.iTime)
            return NULL;
        *ppPrecedes = p;
    }
    // Not found, return NULL
    return NULL;
}
/**************************** insertOrderedLL **********************************
 NodeLL *insertOrderedLL(LinkedList list, Event value);
 * 
 * Purpose:
 *      inserts an event into an ordered linked list
 * 
 * Parameters:
 *      I LinkedList list       Linked List
 *      I Event value           Event typedef struct variable
 * 
 * Notes:
 *      If already exists in list, returns the pointer to that node using searchLL function
 * 
 * Return Value:
 *      NodeLL *
 ******************************************************************************/
NodeLL *insertOrderedLL(LinkedList list, Event value)
{
    //declare variables
    NodeLL *pNew;
    NodeLL *pFind;
    NodeLL *pPrecedes;

    //see if node exists, pass &pPrecedes so searchLL can change it
    pFind = searchLL(list, value.iTime, &pPrecedes);


    //if node exists, insert node before pre-existing node
    if(pFind != NULL)
    {
        pNew = allocNodeLL(list, value);

        pNew->pNext = pPrecedes->pNext;
        pPrecedes->pNext = pNew;
        return pNew;
    }

    // node doesn't exist, allocate new node
    pNew = allocNodeLL(list, value);

    //check for insertion at beginning of list
    // else handles if inserting between two nodes
    if(pPrecedes == NULL)
    {
        pNew->pNext = list->pHead;
        list->pHead = pNew;
    }
    else
    {
        pNew->pNext = pPrecedes->pNext;
        pPrecedes->pNext = pNew;
    }

    return pNew;
}
/******************************** removeLL *************************************
        int removeLL(LinkedList list, Event *pValue);
 * 
 * Purpose:
 *      Removes a node from the front of a linked list and returns event structure
 * 
 * Parameters:
 *      I LinkedList list       Linked List
 *      O Event *pValue         Returned event
 * 
 * Notes:
 *      
 * Return value:
 *      FALSE       If list is empty (before removing node)
 *      TRUE        If successfully removed Node
 * 
 ******************************************************************************/
int removeLL(LinkedList list, Event  *pValue)
{
    //declare variables
    NodeLL *pRemove;

    //Can't remove node from an empty list, so return FALSE
    if(list->pHead == NULL)
        return FALSE;

    //return the element value via pValue
    *pValue = list->pHead->event;

    //set pRemove to point to the node being removed and remove it
    pRemove = list->pHead;
    list->pHead = pRemove->pNext;
    free(pRemove);

    return TRUE;

}
/************************** newQueue *******************************************
 *      Queue newQueue(char szQueueNm[]);
 * Purpose:
 *      Implements a queue using a singly linked list with head and foot pointers
 * 
 * Parameters:
 *      char szQueueNm[]
 * 
 * Return value:
 *      Queue
********************************************************************************/
Queue newQueue(char szQueueNm[])
{
    Queue q = (Queue)malloc(sizeof(QueueImp));
    if (q == NULL)
        errExit("Could not allocate Queue");

    //mark the list as empty
    q->pHead = NULL;    //empty list
    q->pFoot = NULL;    //empty list
    strcpy(q->szQName,szQueueNm);
    return q;
}
/******************************** freeQueue ************************************
 void freeQueue(Queue q)
 * 
 * Parameters:
 *      Queue q         Queue to free
 * 
 ******************************************************************************/
void freeQueue(Queue q)
{
    NodeQ *p;
    NodeQ *qRemove;
    for (p = q->pHead; p != NULL; )
    {
        qRemove = p;
        p = p->pNext;
        free(qRemove);
    }
    free(q);
}
/******************************* insertQ ***************************************
 *      void insertQ(Queue q, QElement element)
 * Purpose:
 *      Inserts a NodeQ in a Queue by pointing a header and footer node
 * Parameters:
 *      I/O Queue q
 *      I   QElement element
 * 
 * ****************************************************************************/
void insertQ(Queue q, QElement element)
{
    NodeQ *pNew;
    pNew = allocNodeQ(q, element);
    if(pNew == NULL)
        errExit("could not allocate NodeQ");

    //check if empty
    if(q->pFoot == NULL)
    {
        //insert first node
        q->pHead = pNew;
        q->pFoot = pNew;
    }
    else
    {
        //insert after foot
        q->pFoot->pNext = pNew;
        q->pFoot = pNew;
    }
}
/******************************* allocNodeQ ************************************
 *      NodeQ *allocNodeQ(Queue q, QElement value);
 * Purpose:
 *      allocates and creates space for a NodeQ holding a value of type QElement
 * Parameters:
 *      I Queue q
 *      I QElement value
 * Returns:
 *      NodeQ 
 *******************************************************************************/
NodeQ *allocNodeQ(Queue q, QElement value)
{
    NodeQ *pNew;
    pNew = (NodeQ*)malloc(sizeof(NodeQ));
    if(pNew == NULL)
        errExit("could not allocate NodeQ");

    pNew->element = value;
    pNew->pNext = NULL;
    return pNew;
}
/******************************* removeQ ***************************************
 int removeQ(Queue q, QElement *pFromQElement)
 * Purpose:
 *      Removes a node from the front of the list and returns the element
 *      via the parameter list
 *      Functionally returns TRUE if an element is returned
 *          FALSE if otherwise(bc queue was empty)
 * Parameters:
 *      I/O Queue q
 *      O   QElement pFromQElement
 * Notes:
 *      Cases to consider:
 *              1. Queue is empty when removeQ is called
 *              2. Queue is empty after removing the last node
 * Returns:
 *      int TRUE/FALSE      if element is returned/if queue was empty
 ******************************************************************************/
int removeQ(Queue q, QElement *pFromQElement)
{
    NodeQ *pRemove;

    //check for empty list
    if(q->pHead == NULL)
        return FALSE;

    pRemove = q->pHead;
    *pFromQElement = pRemove->element;
    q->pHead = pRemove->pNext;

    //Removing the node could make the list empty
    //See if we need to update pFoot, due to empty list
    if(q->pHead == NULL)
        q->pFoot = NULL;
    free(pRemove);
    return TRUE;
}
/******************** processCommandSwitches ***********************************
    void processCommandSwitches(int argc, char *argv[], Simulation sim)
Purpose:
    Checks the syntax of command line arguments and returns the filenames.  
    If any switches are unknown, it exits with an error.
Parameters:
    I   int argc                        count of command line arguments
    I   char *argv[]                    array of command line arguments
    I/O 
Notes:
    If a -? switch is passed, the usage is printed and the program exits
    with USAGE_ONLY.
    If a syntax error is encountered (e.g., unknown switch), the program
    prints a message to stderr and exits with ERR_COMMAND_LINE_SYNTAX.
*******************************************************************************/
void processCommandSwitches(int argc, char *argv[], Simulation sim)
{
    int i;
    // Examine each of the command arguments other than the name of the program.
    for (i = 1; i < argc; i++)
    {
        // check for a switch
        if (argv[i][0] != '-')
        {
            return;
        }
        // determine which switch it is
        switch (argv[i][1])
        {
        case 'v':                   // Query File Name
            sim->bVerbose = TRUE;
            break;

        case 'a':
            sim->cRunType = argv[i][2];
            break;

        case '?':
            exitUsage(USAGE_ONLY, "", "");
            break;
        default:
            exitUsage(i, "expected switch in command line, found ", argv[i]);
        }
    }
}
/******************** exitUsage ************************************************
    void exitUsage(int iArg, char *pszMessage, char *pszDiagnosticInfo)
Purpose:
    In general, this routine optionally prints error messages and diagnostics.
    It also prints usage information.

    If this is an argument error (iArg >= 0), it prints a formatted message 
    showing which argument was in error, the specified message, and
    supplemental diagnostic information.  It also shows the usage. It exits 
    with ERR_COMMAND_LINE.

    If this is a usage error (but not specific to the argument), it prints 
    the specific message and its supplemental diagnostic information.  It 
    also shows the usage and exist with ERR_COMMAND_LINE. 

    If this is just asking for usage (iArg will be -1), the usage is shown.
    It exits with USAGE_ONLY.
Parameters:
    I int iArg                      command argument subscript or control:
                                    > 0 - command argument subscript
                                    0 - USAGE_ONLY - show usage only
                                    -1 - USAGE_ERR - show message and usage
    I char *pszMessage              error message to print
    I char *pszDiagnosticInfo       supplemental diagnostic information
Notes:
    This routine causes the program to exit.
*******************************************************************************/
void exitUsage(int iArg, char *pszMessage, char *pszDiagnosticInfo)
{
    switch (iArg)
    {
        case USAGE_ERR:
            fprintf(stderr, "Error: %s %s\n"
                , pszMessage
                , pszDiagnosticInfo);
            break;
        case USAGE_ONLY:
            break;
        default:
        fprintf(stderr, "Error: bad argument #%d.  %s %s\n"
                , iArg
                , pszMessage
                , pszDiagnosticInfo);
    }
    // print the usage information for any type of command line error
    fprintf(stderr, "p4 -v\n");
    if (iArg == USAGE_ONLY)
        exit(USAGE_ONLY);
    else
        exit(ERR_COMMAND_LINE);
}
/***************************** errExit *****************************************
    void errExit(char szFmt[], ... )
Purpose:
    Prints an error message defined by the printf-like szFmt and the
    corresponding arguments to that function.  The number of 
    arguments after szFmt varies dependent on the format codes in
    szFmt.  
    It also exits the program, returning ERR_EXIT.
Parameters:
    I   char szFmt[]            This contains the message to be printed
                                and format codes (just like printf) for 
                                values that we want to print.
    I   ...                     A variable-number of additional arguments
                                which correspond to what is needed
                                by the format codes in szFmt. 
Returns:
    Returns a program exit return code:  the value of ERR_EXIT.
Notes:
    - Prints "ERROR: " followed by the formatted error message specified 
      in szFmt. 
    - Prints the file path and file name of the program having the error.
      This is the file that contains this routine.
    - Requires including <stdarg.h>
*******************************************************************************/
void errExit(char szFmt[], ... )
{
    va_list args;               // This is the standard C variable argument list type
    va_start(args, szFmt);      // This tells the compiler where the variable arguments
                                // begins.  They begin after szFmt.
    printf("ERROR: ");
    vprintf(szFmt, args);       // vprintf receives a printf format string and  a
                                // va_list argument
    va_end(args);               // let the C environment know we are finished with the
                                // va_list argument
    printf("\n\tEncountered in file %s\n", __FILE__);  // this 2nd arg is filled in by
                                // the pre-compiler
    exit(ERR_EXIT);
}








