/**CFile****************************************************************

	FileName    [circacmd.c]

	SystemName  [ABC: Logic synthesis and verification system.]

	PackageName []

	Synopsis    []

	Author      [Linus Witschen]
	
	Affiliation [Paderborn University]

	Date        [Ver. 1.0. Started - January 25, 2017.]

	Revision    [$Id: .c,v 1.00 2017/01/25 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "circa/circa.h"

#include <stdio.h>
#include <string.h>


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/*=== ioAbc.c ==========================================================*/
extern Abc_Ntk_t *      Io_ReadBlifAsAig( char * pFileName, int fCheck );


static int 				Abc_CircaCommandReplaceByConst0   	( Abc_Frame_t * pAbc, int argc, char ** argv );

static int 				Abc_CircaCommandCrit              	( Abc_Frame_t * pAbc, int argc, char ** argv );
static int 				Abc_CircaCommandCutSize           	( Abc_Frame_t * pAbc, int argc, char ** argv );

static int 				Abc_CircaCommandMarkCrit          	( Abc_Frame_t * pAbc, int argc, char ** argv );
static int 				Abc_CircaCommandUnmarkCrit        	( Abc_Frame_t * pAbc, int argc, char ** argv );
static int 				Abc_CircaCommandShowCrit          	( Abc_Frame_t * pAbc, int argc, char ** argv );

static int 				Abc_CircaCommandAigRewrite        	( Abc_Frame_t * pAbc, int argc, char ** argv );
static int 				Abc_CircaCommandPrecisionScaling  	( Abc_Frame_t * pAbc, int argc, char ** argv );
static int 				Abc_CircaCommandAdc        			( Abc_Frame_t * pAbc, int argc, char ** argv );

static int 				Abc_CircaCommandNtkValidAfter		( Abc_Frame_t * pAbc, int argc, char ** argv );
static int 				Abc_CircaCommandNtkOrPos			( Abc_Frame_t * pAbc, int argc, char ** argv );
static int 				Abc_CircaCommandSeqVerification		( Abc_Frame_t * pAbc, int argc, char ** argv );
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

	Synopsis    []

	Description []
					
	SideEffects []

	SeeAlso     []

***********************************************************************/
void Circa_Init( Abc_Frame_t * pAbc )
{
	Cmd_CommandAdd( pAbc, "CIRCA",      "replace_const0",     	Abc_CircaCommandReplaceByConst0,  1 );

	Cmd_CommandAdd( pAbc, "CIRCA",      "mark_crit",          	Abc_CircaCommandMarkCrit,         1 );
	Cmd_CommandAdd( pAbc, "CIRCA",      "unmark_crit",        	Abc_CircaCommandUnmarkCrit,       1 );
	Cmd_CommandAdd( pAbc, "CIRCA",      "show_crit",          	Abc_CircaCommandShowCrit,         0 );

	Cmd_CommandAdd( pAbc, "CIRCA",      "crit",               	Abc_CircaCommandCrit,             1 );
	Cmd_CommandAdd( pAbc, "CIRCA",      "cut_size",           	Abc_CircaCommandCutSize,          0 );

	Cmd_CommandAdd( pAbc, "CIRCA",      "aig_rewrite",        	Abc_CircaCommandAigRewrite,       1 );
	Cmd_CommandAdd( pAbc, "CIRCA",      "precision_scaling",    Abc_CircaCommandPrecisionScaling, 1 );
	Cmd_CommandAdd( pAbc, "CIRCA",      "adc",        			Abc_CircaCommandAdc,       		1 );

	Cmd_CommandAdd( pAbc, "CIRCA",      "ntk_valid_after",		Abc_CircaCommandNtkValidAfter,	1 );
	Cmd_CommandAdd( pAbc, "CIRCA",      "ntk_or_po",			Abc_CircaCommandNtkOrPos,			1 );
	Cmd_CommandAdd( pAbc, "CIRCA",      "seq_verify",        	Abc_CircaCommandSeqVerification,  1 );
}

/**Function*************************************************************

	Synopsis    []

	Description []
					
	SideEffects []

	SeeAlso     []

***********************************************************************/
void Circa_End( Abc_Frame_t * pAbc )
{

}

/**Function*************************************************************

	Synopsis    []

	Description [Extracts all nodes on the critical path down from the defined node.]
					
	SideEffects [AIG may have single inputs, and, thus, has to be repaired.]

	SeeAlso     []

***********************************************************************/
int Abc_CircaCommandCrit( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pErr;
	Abc_Ntk_t * pNtk, * pNtkRes;
	Abc_Obj_t * pNode;
	int c;

	pNtk = Abc_FrameReadNtk(pAbc);
	pErr = Abc_FrameReadErr(pAbc);

	Extra_UtilGetoptReset();
	while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
	{
		switch ( c )
		{
		case 'h':
			goto usage;
		default:
			goto usage;
		}
	}

	if ( pNtk == NULL )
	{
		fprintf( pErr, "Empty network.\n" );
		return 1;
	}

	if ( !Abc_NtkIsStrash(pNtk) )
	{
		fprintf( pErr, "This command works only for AIGs; run strashing (\"st\").\n" );
		return 0;
	}

	if ( argc != globalUtilOptind + 1 )
	{
		fprintf( pErr, "Wrong number of auguments.\n" );
		goto usage;
	}

	pNode = Abc_NtkFindNode( pNtk, argv[globalUtilOptind] );
	if ( pNode == NULL )
	{
		fprintf( pErr, "Empty node.\n" );
		return 1;
	}

	Abc_CircaNtkMarkCrit( pNtk );
	pNtkRes = Abc_CircaNtkCreateCrit( pNtk, pNode, argv[globalUtilOptind] );
	if ( pNtkRes == NULL )
	{
		fprintf( pErr, "Splitting one node has failed.\n" );
		return 1;
	}
	Abc_NtkCleanMarkA( pNtk );
	// Abc_NtkCleanMarkA( pNtkRes );

	// replace the current network
	Abc_FrameReplaceCurrentNetwork( pAbc, pNtkRes );

	return 0;

usage:
	fprintf( pErr, "usage: crit [-h] <node name>\n" );
	fprintf( pErr, "\t        extracts critical path nodes from network\n" );
	fprintf( pErr, "\t-h    : print the command usage\n");
	return 1;
}

/**Function*************************************************************

	Synopsis    []

	Description []
					
	SideEffects []

	SeeAlso     []

***********************************************************************/
int Abc_CircaCommandShowCrit( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pOut, * pErr;
	Abc_Ntk_t * pNtk;
	int fListNodes;
	int fSeq;
	int fGateNames;
	int fUseReverse;
	int c;

	pNtk = Abc_FrameReadNtk(pAbc);
	pOut = Abc_FrameReadOut(pAbc);
	pErr = Abc_FrameReadErr(pAbc);

	// set defaults
	fListNodes  = 0;
	fSeq        = 0;
	fGateNames  = 0;
	fUseReverse = 1;
	Extra_UtilGetoptReset();
	while ( ( c = Extra_UtilGetopt( argc, argv, "rsglh" ) ) != EOF )
	{
		switch ( c )
		{
		case 'r':
			fUseReverse ^= 1;
			break;
		case 's':
			fSeq ^= 1;
			break;
		case 'g':
			fGateNames ^= 1;
			break;
		case 'l':
			fListNodes ^= 1;
			break;
		default:
			goto usage;
		}
	}

	if ( pNtk == NULL )
	{
		fprintf( pErr, "Empty network.\n" );
		return 1;
	}

	if ( !Abc_NtkIsStrash(pNtk) )
	{
		fprintf( pErr, "This command works only for AIGs; run strashing (\"st\").\n" );
		return 0;
	}

	Abc_CircaNtkMarkCrit( pNtk );

	Abc_NtkShow( pNtk, fGateNames, fSeq, fUseReverse );

	if( fListNodes ) {
		Abc_CircaNtkPrintCritNodes(pOut, pNtk);
	}

	Abc_CircaNtkUnmarkCrit( pNtk );

	return 0;

usage:
	fprintf( pErr, "usage: show_crit [-rsglh]\n" );
	fprintf( pErr, "       visualizes the network structure using DOT and GSVIEW\n" );
#ifdef WIN32
	fprintf( pErr, "       \"dot.exe\" and \"gsview32.exe\" should be set in the paths\n" );
	fprintf( pErr, "       (\"gsview32.exe\" may be in \"C:\\Program Files\\Ghostgum\\gsview\\\")\n" );
#endif
	fprintf( pErr, "\t-s    : toggles visualization of sequential networks [default = %s].\n", fSeq? "yes": "no" );  
	fprintf( pErr, "\t-r    : toggles ordering nodes in reverse order [default = %s].\n", fUseReverse? "yes": "no" );  
	fprintf( pErr, "\t-g    : toggles printing gate names for mapped network [default = %s].\n", fGateNames? "yes": "no" );
	fprintf( pErr, "\t-l    : toggles printing nodes on the critical path.\n");
	fprintf( pErr, "\t-h    : print the command usage.\n");
	return 1;
}

/**Function*************************************************************

	Synopsis    []

	Description []
					
	SideEffects []

	SeeAlso     []

***********************************************************************/
int Abc_CircaCommandMarkCrit( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pOut, * pErr;
	Abc_Ntk_t * pNtk;
	int fListNodes;
	int c;

	pNtk = Abc_FrameReadNtk(pAbc);
	pOut = Abc_FrameReadOut(pAbc);
	pErr = Abc_FrameReadErr(pAbc);

	// set defaults
	fListNodes = 0;
	Extra_UtilGetoptReset();
	while ( ( c = Extra_UtilGetopt( argc, argv, "lh" ) ) != EOF )
	{
		switch ( c )
		{
		case 'l':
			fListNodes ^= 1;
			break;
		case 'h':
			goto usage;
		default:
			goto usage;
		}
	}

	if ( pNtk == NULL )
	{
		fprintf( pErr, "Empty network.\n" );
		return 1;
	}

	if ( !Abc_NtkIsStrash(pNtk) )
	{
		fprintf( pErr, "This command works only for AIGs; run strashing (\"st\").\n" );
		return 0;
	}

	Abc_CircaNtkMarkCrit( pNtk );

	if( fListNodes ) {
		Abc_CircaNtkPrintCritNodes(pOut, pNtk);
	}

	return 0;

usage:
	fprintf( pErr, "usage: mark_crit [-lh]\n" );
	fprintf( pErr, "\t        prints information about critical path and marks nodes on it.\n" );
	fprintf( pErr, "\t        [Remark:] Do not forget to unmark nodes before replacing nodes, it will fail otherwise.\n" );
	fprintf( pErr, "\t-l    : toggles printing nodes on the critical path.\n");
	fprintf( pErr, "\t-h    : print the command usage.\n");
	return 1;
}

/**Function*************************************************************

	Synopsis    []

	Description []
					
	SideEffects []

	SeeAlso     []

***********************************************************************/
int Abc_CircaCommandUnmarkCrit( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pErr;
	Abc_Ntk_t * pNtk;
	int c;

	pNtk = Abc_FrameReadNtk(pAbc);
	pErr = Abc_FrameReadErr(pAbc);

	// set defaults
	Extra_UtilGetoptReset();
	while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
	{
		switch ( c )
		{
		case 'h':
			goto usage;
		default:
			goto usage;
		}
	}

	if ( pNtk == NULL )
	{
		fprintf( pErr, "Empty network.\n" );
		return 1;
	}

	if ( !Abc_NtkIsStrash(pNtk) )
	{
		fprintf( pErr, "This command works only for AIGs; run strashing (\"st\").\n" );
		return 0;
	}

	Abc_CircaNtkUnmarkCrit( pNtk );

	return 0;

usage:
	fprintf( pErr, "usage: unmark_crit [-h]\n" );
	fprintf( pErr, "\t-h    : print the command usage.\n");
	return 1;
}

/**Function*************************************************************

	Synopsis    []

	Description [Command takes a node as parameter and replaces it by const0.]
					
	SideEffects [Other nodes may be removed.]

	SeeAlso     []

***********************************************************************/
int Abc_CircaCommandReplaceByConst0( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pErr;
	Abc_Ntk_t * pNtk;
	Abc_Obj_t * pNode;

	int c;

	pNtk = Abc_FrameReadNtk(pAbc);
	pErr = Abc_FrameReadErr(pAbc);

	// set defaults
	Extra_UtilGetoptReset();
	while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
	{
		switch ( c )
		{
		case 'h':
			goto usage;
		default:
			goto usage;
		}
	}

	if ( pNtk == NULL )
	{
		fprintf( pErr, "Empty network.\n" );
		return 1;
	}

	if ( !Abc_NtkIsStrash(pNtk) )
	{
		fprintf( pErr, "This command works only for AIGs; run strashing (\"st\").\n" );
		return 0;
	}

	if ( argc != globalUtilOptind + 1 )
	{
		fprintf( pErr, "Wrong number of auguments.\n" );
		goto usage;
	}

	pNode = Abc_NtkFindNode( pNtk, argv[globalUtilOptind] );
	if ( pNode == NULL )
	{
		fprintf( pErr, "Empty node.\n" );
		return 1;
	}
	else {
		Abc_CircaNodeReplaceByConst0( pNtk, pNode );
	}

	return 0;

usage:
	fprintf( pErr, "usage: replace_const0 [-h] <node> \n" );
	fprintf( pErr, "\t        The node is replaced by const0.\n" );
	fprintf( pErr, "\t-h    : print the command usage.\n");
	return 1;
}

/**Function*************************************************************

	Synopsis    []

	Description [Command takes a node as parameter and determines its cut size.]
					
	SideEffects []

	SeeAlso     []

***********************************************************************/
int Abc_CircaCommandCutSize( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pErr;
	Abc_Ntk_t * pNtk;
	Abc_Obj_t * pNode;
	Vec_Ptr_t* vCut;
	int c;
	int nNodeSizeMax;
	int nConeSizeMax;

	pNtk = Abc_FrameReadNtk(pAbc);
	pErr = Abc_FrameReadErr(pAbc);

	// set defaults
	nNodeSizeMax = 10;
	nConeSizeMax = ABC_INFINITY;
	Extra_UtilGetoptReset();
	while ( ( c = Extra_UtilGetopt( argc, argv, "NCh" ) ) != EOF )
	{
		switch ( c )
		{
		case 'N':
			if ( globalUtilOptind >= argc )
			{
				fprintf( pErr, "Command line switch \"-N\" should be followed by an integer.\n" );
				goto usage;
			}
			nNodeSizeMax = atoi(argv[globalUtilOptind]);
			globalUtilOptind++;
			if ( nNodeSizeMax < 0 ) 
				goto usage;
			break;
		case 'C':
			if ( globalUtilOptind >= argc )
			{
				fprintf( pErr, "Command line switch \"-C\" should be followed by an integer.\n" );
				goto usage;
			}
			nConeSizeMax = atoi(argv[globalUtilOptind]);
			globalUtilOptind++;
			if ( nConeSizeMax < 0 ) 
				goto usage;
			break;
		case 'h':
			goto usage;
		default:
			goto usage;
		}
	}

	if ( pNtk == NULL )
	{
		fprintf( pErr, "Empty network.\n" );
		return 1;
	}

	if ( !Abc_NtkIsStrash(pNtk) )
	{
		fprintf( pErr, "This command works only for AIGs; run strashing (\"st\").\n" );
		return 0;
	}

	if ( argc != globalUtilOptind + 1 )
	{
		fprintf( pErr, "Wrong number of auguments.\n" );
		goto usage;
	}

	pNode = Abc_NtkFindNode( pNtk, argv[globalUtilOptind] );
	if ( pNode == NULL )
	{
		fprintf( pErr, "Empty node.\n" );
		return 1;
	}
	else {
		vCut = Abc_CircaNodeCutSize( pNtk, pNode, nNodeSizeMax, nConeSizeMax );
	}

	Vec_PtrFree( vCut );

	return 0;

usage:
	fprintf( pErr, "usage: cut_size [-NCh] <node> \n" );
	fprintf( pErr, "\t-N num : the max size of the cut to be computed [default = %d]\n", nNodeSizeMax );  
	fprintf( pErr, "\t-C num : the max support of the containing cone [default = %d]\n", nConeSizeMax );  
	fprintf( pErr, "\t        The cut size of the node is calculated.\n" );
	fprintf( pErr, "\t-h    : print the command usage.\n");
	return 1;
}

/**Function*************************************************************

	Synopsis    []

	Description [This function performs AIG Re-writing on the given network, i.e. 
				 it replaces nodes on the critical path by const. 0.]
					
	SideEffects []

	SeeAlso     [Chandrasekharan et al. 16: Approximation-aware Rewriting of AIGs for Error Tolerant Applications]

***********************************************************************/
int Abc_CircaCommandAigRewrite( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pErr;
	Abc_Ntk_t * pNtkOrig, * pNtkApp, * pNtkTest, * pNtkQc, * pNtkPc;
	Abc_Obj_t * pRoot;
	Abc_Obj_t * pCand;
	// Holds roots of all crit. paths
	Vec_Ptr_t * vCritRoots;
	//  vvCrits
	//  Is a vector of size #RootsCrit that holds pointers to vectors that hold all nodes on the particular crit. path. 
	Vec_Ptr_t * vvCrits;
	// Holds cuts of the selected crit. path
	Vec_Ptr_t * vvCuts;
	Vec_Ptr_t * vNodes;
	Vec_Ptr_t * vCands;

	char* pFileName;

	int nCritRoots;

	Circa_EffortType_t effortLvl;

	int fReplaced;
	int fContinue;
	int nChangesRun;
	int nChangesTotal;
	int nNodesStart;
	int nNodesEnd;
	int fPass;

	int i;
	int k;
	int l;
	int c;

	int nNodeSizeMax;
	int nConeSizeMax;

	// pNtkOrig = Abc_FrameReadNtk(pAbc);
	pErr = Abc_FrameReadErr(pAbc);

	// set defaults
	nNodeSizeMax = 10;
	nConeSizeMax = ABC_INFINITY;
	effortLvl = CIRCA_EFFORT_MIN;
	Extra_UtilGetoptReset();
	while ( ( c = Extra_UtilGetopt( argc, argv, "NCEeh" ) ) != EOF )
	{
		switch ( c )
		{
		case 'N':
			if ( globalUtilOptind >= argc )
			{
				fprintf( pErr, "Command line switch \"-N\" should be followed by an integer.\n" );
				goto usage;
			}
			nNodeSizeMax = atoi(argv[globalUtilOptind]);
			globalUtilOptind++;
			if ( nNodeSizeMax < 0 ) 
				goto usage;
			break;
		case 'C':
			if ( globalUtilOptind >= argc )
			{
				fprintf( pErr, "Command line switch \"-C\" should be followed by an integer.\n" );
				goto usage;
			}
			nConeSizeMax = atoi(argv[globalUtilOptind]);
			globalUtilOptind++;
			if ( nConeSizeMax < 0 ) 
				goto usage;
			break;
		case 'E':
			if ( globalUtilOptind >= argc )
			{
				fprintf( pErr, "Command line switch \"-E\" should be followed by an integer.\n" );
				goto usage;
			}
			effortLvl = atoi(argv[globalUtilOptind]);
			globalUtilOptind++;
			break;
		case 'h':
			goto usage;
		default:
			goto usage;
		}
	}
	
	// Read-in circuits
	if ( argc-1 != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    pNtkQc = Io_ReadBlifAsAig( pFileName, 1 );
    if ( pNtkQc == NULL )
    {
		fprintf( pErr, "Empty quality function circuit.\n" );
        return 1;
    }
    globalUtilOptind++;

	if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    pNtkOrig = Io_ReadBlifAsAig( pFileName, 1 );
	if ( pNtkOrig == NULL )
	{
		fprintf( pErr, "Empty original circuit.\n" );
		return 1;
	}

	// Store original network and use copy for modifications
	// If modifications lead to accepable results => Keep it in the end
	pNtkApp = Abc_NtkDup( pNtkOrig );

	// Initialize random numbers generator
	srand(time(NULL));

	// Give flags a initial value so that the loop starts
	nNodesStart = Abc_NtkNodeNum( pNtkApp );
	nNodesEnd = 0;
	nChangesTotal = 0;
	fContinue = 1;
	for( i=0; fContinue; i++ )
	{
		// Mark all nodes on the critical paths
		Abc_CircaNtkMarkCrit( pNtkApp );

		// Extract root nodes of the critical paths
		vCritRoots = Abc_CircaNodeCollectCrit( pNtkApp );
		// Set number of crit. paths/root nodes
		nCritRoots = Vec_PtrSize( vCritRoots );

		// Store all nodes on critical path from root node for each root node
		vvCrits = Vec_PtrAlloc( Vec_PtrSize(vCritRoots)*sizeof(Vec_Ptr_t*) );
		Vec_PtrForEachEntry( Abc_Obj_t*, vCritRoots, pRoot, k )
		{
			Vec_Ptr_t* vCrit = Abc_NtkDfsNodesWithMarkA( pNtkApp, &pRoot, 1 );
			Vec_PtrPush( vvCrits, vCrit );
		}

		// Shuffle critical paths in vector
		if( Vec_PtrSize(vvCrits) > 1 )
		{
			// Do it for half of the vector size
			int nSize = Vec_PtrSize(vvCrits);
			for(k=0; k < nSize; k++)
			{
				// Generate a number between 0 and Vec_PtrSize(vvCrits)
				int r = rand() % nSize;
				// Place entry at the end
				void* pEntryTmp = Vec_PtrEntry( vvCrits, r );
				Vec_PtrRemove( vvCrits, pEntryTmp );
				Vec_PtrPush( vvCrits, pEntryTmp );
			}
		}

		// Clear Mark A since it will be used for cuts
		Abc_NtkCleanMarkA( pNtkApp );
		// Calculate cuts for each node
		// Reset nChangesRun counter
		nChangesRun = 0;
		// If vector is empty do not even try to do stuff
		fContinue = nCritRoots;
		for( k = 0; fContinue && (k < Vec_PtrSize(vvCrits)) && (((vNodes) = Vec_PtrEntry(vvCrits, k)), 1); k++ )
		{
			printf("\n*********************************** Iteration % d Crit. path %d ***********************************\n", i, k);

			// Find and store cuts of the crit. path stored on vNodes
			vvCuts = Abc_CircaNodeCuts( pNtkApp, vNodes, nNodeSizeMax, nConeSizeMax );
			// Take node with smallest cut size and replace it by const0
			printf("Looking at %d nodes.\n", Vec_PtrSize(vvCuts));
			int nNodesRejected = 0;
			while( Vec_PtrSize(vvCuts) )
			{
				// Duplicate network to test replacement
				pNtkTest = Abc_NtkDup( pNtkApp );

				// Take node with smallest cut size
				vCands = Vec_PtrPop( vvCuts );
				// Last element is root of the cut. After that the cut sorted by level follows.
				pCand = Vec_PtrPop( vCands );

				// If node's type in ABC_OBJ_NONE then it has been deleted in a previous iteration => Skip
				// Also skip if copy is not valid
				if( (Abc_ObjType(pCand) != ABC_OBJ_NONE) && Abc_ObjCopyCond( pCand ) )
				{
					// Copy of candidate is node in the test network
					Abc_Obj_t* pNodeTest = Abc_ObjCopy( pCand );

					fReplaced = 0;
					if( Abc_ObjType( pNodeTest ) == ABC_OBJ_NODE )
					{
						// Replace candidate by const0
						// printf("Trying to replace node %d from %s\n", Abc_ObjId(pNodeTest), Abc_NtkName(Abc_ObjNtk(pNodeTest)));
						fReplaced = Abc_CircaNodeReplaceByConst0( pNtkTest, pNodeTest );
					}
					else {
						printf("Node for replacement is invalid, or does not exist anymore.\n");
					}

					// Check quality of transformed circuit
					if( fReplaced )
					{
						// Create property checker
						pNtkPc = Abc_CircaBuildPropertyChecker( pNtkOrig, pNtkTest, pNtkQc );

						int fVerbose   = 0;
					    int nConfLimit = 200000;
					    int nInsLimit  = 0;
						fPass = Abc_NtkMiterSat( pNtkPc, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)nInsLimit, fVerbose, NULL, NULL );
						// printf("\t\t\t\t\t\t\t\t *************************************    fPass %d\n", fPass);
						Abc_NtkDelete( pNtkPc );

						// Stop replacing nodes, error constraints are violated
						if( fPass == -1 )
						{
							printf("\tSAT-Solver timed out. Node %s will not be replaced.\n", Abc_ObjName( pCand ));
						}
						else if( fPass == 0 )
						{
							nNodesRejected++;
							// printf("\t%s is violating quality constraints. Node will not be replaced.\n", Abc_ObjName( pCand ));
						}
						else if( fPass == 1 )
						{
							// Circuit is good, so go ahead and replace it in the "real" circuit
							// printf("\tQC passed. Replacing %s\n", Abc_ObjName( pCand ));
							Abc_CircaNodeReplaceByConst0( pNtkApp, pCand );

							// Mark that changes have been made
							nChangesRun++;
						}
						// Delete test network
						Abc_NtkDelete( pNtkTest );
					}
				}
			}

			// Free memory for a new round
			Vec_PtrForEachEntry( Vec_Ptr_t*, vvCuts, vCands, l )
			{
				Vec_PtrFree( vCands );
			}
			Vec_PtrFree( vvCuts );

			// Should approximation should be continued?
			// k+1 because look for the next iteration. Is it needed?
			fContinue = !Abc_CircaEnoughEffort( k+1, nCritRoots, nChangesRun, effortLvl );
			printf("\nnChanges %d\tnRejections %d\n\n", nChangesRun, nNodesRejected);
			// nChangesTotal += nChangesRun;
			nChangesRun = 0;
		}


		// Cleanup
		Vec_PtrFree( vCritRoots );
		Vec_PtrForEachEntry( Vec_Ptr_t*, vvCrits, vNodes, k )
		{
			Vec_PtrFree( vNodes );
		}
		Vec_PtrFree( vvCrits );

		Abc_AigCleanup( (Abc_Aig_t *)pNtkApp->pManFunc );

		if ( !Abc_AigCheck( (Abc_Aig_t *)pNtkApp->pManFunc ) )
			fprintf( stdout, "(Abc_CircaCommandAigRewrite): Network check has failed.\n" );
	}

	nNodesEnd = Abc_NtkNodeNum( pNtkApp );

	nChangesTotal = nNodesStart - nNodesEnd;
	printf("\nTotal changes %d. Started with %d nodes. Ended with %d nodes.\n\n", nChangesTotal, nNodesStart, nNodesEnd);

	Abc_FrameReplaceCurrentNetwork( pAbc, pNtkApp );

	return 0;

usage:
	fprintf( pErr, "usage: aig_rewrite [-NCEeh] <quality function circuit> <original circuit>\n" );
	fprintf( pErr, "\t-N num : the max size of the cut to be computed [default = %d]\n", nNodeSizeMax );  
	fprintf( pErr, "\t-C num : the max support of the containing cone [default = %d]\n", nConeSizeMax );  
	fprintf( pErr, "\t-E num : the effort level or number in range [0;2] [default = %d]\n", effortLvl );  
	fprintf( pErr, "\t         0 -> 1 crit. path.\n");
	fprintf( pErr, "\t         1 -> Go through all crit. paths once.\n");
	fprintf( pErr, "\t         2 -> Go through all crit. paths as long as changes can be applied.\n");
	fprintf( pErr, "\t        Prototype implementation of Chandrasekharan.\n" );
	fprintf( pErr, "\t-h    : print the command usage.\n");
	return 1;
}


/**Function*************************************************************

	Synopsis    []

	Description [Approximate Don't Cares (ADC) are used to approximate the given circuit.]
					
	SideEffects []

	SeeAlso     [Venkataramani et al. '12: SALSA: Systematic logic synthesis of approximate circuits]

***********************************************************************/
int Abc_CircaCommandAdc( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pErr;
	Abc_Ntk_t * pNtkOrig, * pNtkMiter, * pNtkQcPos, * pNtkQcNeg, * pNtkQc;
	Abc_Obj_t * pNode, * pObj;
	Vec_Ptr_t * vLeaves = Vec_PtrAlloc( 100 );
	unsigned puTruth[256];

	char* pFileName;

	int i;
	int k;
	int l;
	int c;

	for(i = 0; i < 256; ++i)
	{
		puTruth[i] = i;
	}

	pErr = Abc_FrameReadErr(pAbc);

	// set defaults
	Extra_UtilGetoptReset();
	while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
	{
		switch ( c )
		{
		case 'h':
			goto usage;
		default:
			goto usage;
		}
	}
	
	// Read-in circuits
	if ( argc != globalUtilOptind + 1 )
        goto usage;
    // get the input file name
    pFileName = argv[globalUtilOptind];
    pNtkMiter = Io_ReadBlifAsAig( pFileName, 1 );
    if ( pNtkMiter == NULL )
    {
		fprintf( pErr, "Empty quality function circuit.\n" );
        return 1;
    }
    // globalUtilOptind++;

    assert( Abc_NtkIsStrash(pNtkMiter) );

    Abc_Obj_t* pOld;
    Abc_Obj_t* pNew;

    // pOld = Abc_NtkPi( pNtkMiter, 0 );
    // pNew = Abc_AigConst1( pNtkMiter );
    // Abc_AigReplace( (Abc_Aig_t *)pNtkMiter->pManFunc, pOld, pNew, 1 );
    // pOld = Abc_NtkPi( pNtkMiter, 8 );
    // pNew = Abc_ObjNot( Abc_AigConst1( pNtkMiter ) );
    // Abc_AigReplace( (Abc_Aig_t *)pNtkMiter->pManFunc, pOld, pNew, 1 );

	// Abc_NtkShow( pNtkMiter, 0,0,0 );


	int nCutMax = 9;
	int nLevels = 9;
	int fVerbose = 1;
	int fVeryVerbose = 1;
	Odc_Man_t * pOdcMan = Abc_NtkDontCareAlloc( nCutMax, nLevels, fVerbose, fVeryVerbose );

	Abc_ManCut_t * pManCut = Abc_NtkManCutStart( nCutMax, 100000, 100000, 100000 );

	pNode = Abc_NtkObj( pNtkMiter, Abc_NtkNodeNum( pNtkMiter )-1) ;
    vLeaves = Abc_NodeFindCut( pManCut, pNode, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pObj, i )
    	printf("%s\n", Abc_ObjName(pObj));
	int res = Abc_NtkDontCareCompute( pOdcMan, pNode, vLeaves, puTruth );
	printf("Result %d\n", res);

	Abc_AigCleanup( (Abc_Aig_t *)pNtkMiter->pManFunc );

	if ( !Abc_AigCheck( (Abc_Aig_t *)pNtkMiter->pManFunc ) )
		fprintf( stdout, "(adc): Network check has failed.\n" );

	return 0;

usage:
	fprintf( pErr, "usage: adc [-h] <circuit.blif>\n" );
	fprintf( pErr, "\t-h    : print the command usage.\n");
	return 1;
}

/**Function*************************************************************

	Synopsis    []

	Description [This command checks after how many time frames the 
				 circuit is ready with all of its computations. It checks the 
				 valid flag for this.]
					
	SideEffects []

	SeeAlso     []

***********************************************************************/
int Abc_CircaCommandNtkValidAfter( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pErr;
	Abc_Ntk_t * pNtk;
	Abc_Obj_t * pObj;
	char* name;
	int fIsTautology;
	int nValid;
	int i, k;
	int c;

	pNtk = Abc_FrameReadNtk(pAbc);
	pErr = Abc_FrameReadErr(pAbc);

	// set defaults
	Extra_UtilGetoptReset();
	int fComplPos = 1;
	int fVerbose = 0;
	int fInitial = 1;
	int nConfLimit = 100000;
    int nInsLimit  = 0;
	while ( ( c = Extra_UtilGetopt( argc, argv, "CIcivh" ) ) != EOF )
	{
		switch ( c )
		{
		case 'C':
            if ( globalUtilOptind >= argc )
            {
                fprintf( pErr, "Command line switch \"-C\" should be followed by an integer.\n" );
                goto usage;
            }
            nConfLimit = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nConfLimit < 0 )
                goto usage;
            break;
        case 'I':
            if ( globalUtilOptind >= argc )
            {
                fprintf( pErr, "Command line switch \"-I\" should be followed by an integer.\n" );
                goto usage;
            }
            nInsLimit = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nInsLimit < 0 )
                goto usage;
            break;
		case 'c':
			fComplPos ^= 1;
			break;
		case 'i':
			fInitial ^= 1;
			break;
		case 'v':
			fVerbose ^= 1;
			break;
		case 'h':
			goto usage;
		default:
			goto usage;
		}
	}

	if ( pNtk == NULL )
	{
		fprintf( pErr, "Empty network.\n" );
		return 1;
	}

	if ( !Abc_NtkIsStrash(pNtk) )
	{
		fprintf( pErr, "This command works only for AIGs; run strashing (\"st\").\n" );
		return 0;
	}

	if ( argc != globalUtilOptind + 1 )
	{
		fprintf( pErr, "Wrong number of arguments.\n" );
		goto usage;
	}

	// Get name for "valid" output
	name = argv[globalUtilOptind];
	for(i = 0; name[i] != '\0'; ++i);
	int nameSize = i;

	nValid = 1;
	fIsTautology = 0;
	Vec_Ptr_t* vValids = Vec_PtrAlloc( 1 );
 	while( !fIsTautology )
 	{
 		Abc_Ntk_t* pNtkRes  = Abc_NtkFrames( pNtk, nValid, fInitial, fVerbose );

 		// Delete all other outputs
		for ( k = 0; (k < Abc_NtkCoNum(pNtkRes)) && (((pObj) = Abc_NtkCo(pNtkRes, k)), 1); k++ )
	 	{
			if( strncmp(name, Abc_ObjName(pObj), nameSize) != 0 )
			{	
				Abc_NtkDeleteObj( pObj );
				k--;
			}
			else
			{
				Vec_PtrPush( vValids, pObj );
			}
	 	}

		Abc_Ntk_t * pNtkMiter = Abc_CircaAndAllPos( pNtkRes, fComplPos );

		Abc_AigCleanup( (Abc_Aig_t *)pNtkMiter->pManFunc );

		if ( !Abc_AigCheck( (Abc_Aig_t *)pNtkMiter->pManFunc ) )
			fprintf( stdout, "(Abc_CircaCommandNtkValidAfter): Network check has failed.\n" );

		//  SAT-Solver
	    // Returns -1 if timed out; 0 if SAT; 1 if UNSAT.
		fIsTautology = Abc_NtkMiterSat( pNtkMiter, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)nInsLimit, fVerbose, NULL, NULL );
		// Increment frame
		nValid++;


		// Abc_NtkShow( pNtkRes, 0,0,1 );
		// return 0;
		Abc_NtkDelete( pNtkRes );
		Abc_NtkDelete( pNtkMiter );

		// If SAT timed-out, exit
		if( fIsTautology == -1 )
		{
			// Signal error 
			printf("-1\n");
			return 0;
		}
 	}
 	// Decrement by one for last iteration
 	nValid--;
 	Abc_Ntk_t* pNtkRes  = Abc_NtkFrames( pNtk, nValid, fInitial, fVerbose );

 	// Result can be read from pipe
	printf("%d\n", nValid);

	return 0;

usage:
	fprintf( pErr, "usage: ntk_valid_after [-CIcivh] <name of \"valid\" output> \n" );
	fprintf( pErr, "\t        Computes after how many cycles the network's ready/valid flag is valid. " );
	fprintf( pErr, "\t        ANDs valid output from each timestep. After SAT-Solver finds the output to be unsatisfiable, it is a tautology.\n" );
	fprintf( pErr, "\t-c    : toogles complement of AND inputs. [default = %d]\n", fComplPos );
	fprintf( pErr, "\t-C num : limit on the number of conflicts [default = %d]\n",    nConfLimit );
    fprintf( pErr, "\t-I num : limit on the number of inspections [default = %d]\n", nInsLimit );
	fprintf( pErr, "\t-i    : toogles initial values. [default = %d]\n", fInitial );
	fprintf( pErr, "\t-v    : toogle verbose output.[default = %d]\n", fVerbose);
	fprintf( pErr, "\t-h    : print the command usage.\n");
	return 1;
}

/**Function*************************************************************

	Synopsis    []

	Description [This command takes specified output and connects
				 it to one big OR.]
					
	SideEffects []

	SeeAlso     []

***********************************************************************/
int Abc_CircaCommandNtkOrPos( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pErr;
	Abc_Ntk_t * pNtk;
	Abc_Obj_t * pObj;
	char* name;
	int fUnsat;
	int i, k;
	int c;

	pNtk = Abc_FrameReadNtk(pAbc);
	pErr = Abc_FrameReadErr(pAbc);

	// set defaults
	Extra_UtilGetoptReset();
	int fComplPos = 0;
	int fProveUnsat = 1;
	int fVerbose = 0;
	int nConfLimit = 100000;
    int nInsLimit  = 0;
	while ( ( c = Extra_UtilGetopt( argc, argv, "CIcpvh" ) ) != EOF )
	{
		switch ( c )
		{
		case 'C':
            if ( globalUtilOptind >= argc )
            {
                fprintf( pErr, "Command line switch \"-C\" should be followed by an integer.\n" );
                goto usage;
            }
            nConfLimit = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nConfLimit < 0 )
                goto usage;
            break;
        case 'I':
            if ( globalUtilOptind >= argc )
            {
                fprintf( pErr, "Command line switch \"-I\" should be followed by an integer.\n" );
                goto usage;
            }
            nInsLimit = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nInsLimit < 0 )
                goto usage;
            break;
		case 'c':
			fComplPos ^= 1;
			break;
		case 'p':
			fProveUnsat ^= 1;
			break;
		case 'v':
			fVerbose ^= 1;
			break;
		case 'h':
			goto usage;
		default:
			goto usage;
		}
	}

	if ( pNtk == NULL )
	{
		fprintf( pErr, "Empty network.\n" );
		return 1;
	}

	if ( !Abc_NtkIsStrash(pNtk) )
	{
		fprintf( pErr, "This command works only for AIGs; run strashing (\"st\").\n" );
		return 0;
	}

	if ( argc != globalUtilOptind + 1 )
	{
		fprintf( pErr, "Wrong number of arguments.\n" );
		goto usage;
	}

	// Get output name
	name = argv[globalUtilOptind];
	for(i = 0; name[i] != '\0'; ++i);
	int nameSize = i;

	Vec_Ptr_t* vPos = Vec_PtrAlloc( 1 );
	// Delete all other outputs
	for ( k = 0; (k < Abc_NtkCoNum(pNtk)) && (((pObj) = Abc_NtkCo(pNtk, k)), 1); k++ )
 	{
		if( (strncmp(name, Abc_ObjName(pObj), nameSize) != 0) && (Abc_ObjType(pObj) == ABC_OBJ_PO) )
		{	
			Abc_NtkDeleteObj( pObj );
			k--;
		}
		else
		{
			Vec_PtrPush( vPos, pObj );
		}
 	}

	Abc_Ntk_t * pNtkRes = Abc_CircaOrAllPos( pNtk, fComplPos );

	Abc_AigCleanup( (Abc_Aig_t *)pNtkRes->pManFunc );

	if ( !Abc_AigCheck( (Abc_Aig_t *)pNtkRes->pManFunc ) )
		fprintf( stdout, "(ntk_or_pos): Network check has failed.\n" );

	if( fProveUnsat )
	{
		//  SAT-Solver
	    // Returns -1 if timed out; 0 if SAT; 1 if UNSAT.
		fUnsat = Abc_NtkMiterSat( pNtkRes, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)nInsLimit, fVerbose, NULL, NULL );
		printf("%d\n", fUnsat);
	}

	// replace the current network
	Abc_FrameReplaceCurrentNetwork( pAbc, pNtkRes );

	return 0;

usage:
	fprintf( pErr, "usage: ntk_or_po [-cpvh] <name of PO> \n" );
	fprintf( pErr, "\t        Deletes all POs except the specified ones and ORs them.\n" );
	fprintf( pErr, "\t-c    : toogles complement of ORed inputs. [default = %d]\n", fComplPos );
	fprintf( pErr, "\t-C num : limit on the number of conflicts [default = %d]\n",    nConfLimit );
    fprintf( pErr, "\t-I num : limit on the number of inspections [default = %d]\n", nInsLimit );
	fprintf( pErr, "\t-p    : toogles proves resulting circuit for unsatisfiability. [default = %d]\n", fProveUnsat );
	fprintf( pErr, "\t-v    : toogle verbose output.[default = %d]\n", fVerbose);
	fprintf( pErr, "\t-h    : print the command usage.\n");
	return 1;
}

/**Function*************************************************************

	Synopsis    []

	Description [This command checks after how many time frames the 
				 circuit is ready with its computation. It checks the 
				 valid flag for this.]
					
	SideEffects []

	SeeAlso     []

***********************************************************************/
int Abc_CircaCommandSeqVerification( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pErr;
	Abc_Ntk_t * pNtk;
	Abc_Obj_t * pObj;
	char* name;
	int fPass;
	int i;
	int c;

	pNtk = Abc_FrameReadNtk(pAbc);
	pErr = Abc_FrameReadErr(pAbc);

	// set defaults
	Extra_UtilGetoptReset();
	int fLowActive = 0;
	while ( ( c = Extra_UtilGetopt( argc, argv, "lh" ) ) != EOF )
	{
		switch ( c )
		{
		case 'l':
			fLowActive ^= 1;
			break;
		case 'h':
			goto usage;
		default:
			goto usage;
		}
	}

	if ( pNtk == NULL )
	{
		fprintf( pErr, "Empty network.\n" );
		return 1;
	}

	if ( !Abc_NtkIsStrash(pNtk) )
	{
		fprintf( pErr, "This command works only for AIGs; run strashing (\"st\").\n" );
		return 0;
	}

	if ( argc != globalUtilOptind )
	{
		fprintf( pErr, "Wrong number of arguments.\n" );
		goto usage;
	}

	printf("*****************************************************\n");
 	Abc_NtkForEachCo( pNtk, pObj, i )
		printf("Name %s\n", Abc_ObjName(pObj));
	name = "q_";
	// Delete all other outputs
	for ( i = 0; (i < Abc_NtkCoNum(pNtk)) && (((pObj) = Abc_NtkCo(pNtk, i)), 1); i++ )
 	{
		// printf("Name %s %s\n", Abc_ObjName(pObj), name);
		if( strncmp(name, Abc_ObjName(pObj), 2) != 0 )
		{	
			// printf("REMOVED\n");
			Abc_NtkDeleteObj( pObj );
			i--;
		}
 	}
 	printf("***************************************************** PIs\n");
 	Abc_NtkForEachPi( pNtk, pObj, i )
		printf("Name %s\n", Abc_ObjName(pObj));
 	printf("***************************************************** POs\n");
 	Abc_NtkForEachPo( pNtk, pObj, i )
		printf("Name %s\n", Abc_ObjName(pObj));

	// Build miter
	Abc_Ntk_t * pNtkMiter = Abc_CircaOrAllPos( pNtk, 0 );
	printf("***************************************************** POs\n");
 	Abc_NtkForEachPo( pNtkMiter, pObj, i )
		printf("Name %s\n", Abc_ObjName(pObj));

	Abc_AigCleanup( (Abc_Aig_t *)pNtkMiter->pManFunc );

	if ( !Abc_AigCheck( (Abc_Aig_t *)pNtkMiter->pManFunc ) )
		fprintf( stdout, "(Abc_CircaCommandSeqVerification): Network check has failed.\n" );

	//  SAT-Solver
	int fVerbose   = 1;
    int nConfLimit = 100000;
    int nInsLimit  = 0;
	fPass = Abc_NtkMiterSat( pNtkMiter, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)nInsLimit, fVerbose, NULL, NULL );

	printf("Pass %d\n", fPass);

	// replace the current network
	Abc_FrameReplaceCurrentNetwork( pAbc, pNtkMiter );

	return 0;

usage:
	fprintf( pErr, "usage: seq_verify [-lh]\n" );
	fprintf( pErr, "\t        Checks the sequential correctness of the circuit.\n" );
	fprintf( pErr, "\t-l    : output is low active. [default = %d]\n", fLowActive );
	fprintf( pErr, "\t-h    : print the command usage.\n");
	return 1;
}



/**Function*************************************************************

	Synopsis    []

	Description [Command takes a node as parameter and determines its cut size.]
					
	SideEffects []

	SeeAlso     []

***********************************************************************/
int Abc_CircaCommandPrecisionScaling( Abc_Frame_t * pAbc, int argc, char ** argv )
{
	FILE * pErr;
	Abc_Ntk_t * pNtk, * pNtkOrig;
	Abc_Obj_t* pPo;
	int c;
	int i;
	int nNodesStart;
	int nNodesEnd;
	int* pMask;
	int fConst0;

	pNtkOrig = Abc_FrameReadNtk(pAbc);
	pErr = Abc_FrameReadErr(pAbc);

	// set defaults
	fConst0 = 1;
	Extra_UtilGetoptReset();
	while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
	{
		switch ( c )
		{
		// case 'c':
		// 	fConst0 ^= 1;
		// 	break;
		case 'h':
			goto usage;
		default:
			goto usage;
		}
	}

	if ( argc != globalUtilOptind + 1 )
	{
		fprintf( pErr, "Wrong number of arguments.\n" );
		goto usage;
	}

	if ( pNtkOrig == NULL )
	{
		fprintf( pErr, "Empty network.\n" );
		return 1;
	}

	if ( !Abc_NtkIsStrash(pNtkOrig) )
	{
		fprintf( pErr, "This command works only for AIGs; run strashing (\"st\").\n" );
		return 0;
	}

	if ( argc != globalUtilOptind + 1 )
	{
		fprintf( pErr, "Wrong number of auguments.\n" );
		goto usage;
	}

	pNtk = Abc_NtkDup( pNtkOrig );
	nNodesStart = Abc_NtkNodeNum( pNtk );

	// Get mask
	pMask = (int*)calloc( Abc_NtkPoNum(pNtk), sizeof(int));
	char* maskTmp = argv[globalUtilOptind];
	for(i = 0; maskTmp[i] != '\0'; ++i)
	{
		if( maskTmp[i] == '1' )
			pMask[i] = 1;
	}

	Abc_NtkForEachCo( pNtk, pPo, i )
	{
		if( pMask[i] )
		{
			// Replace driver node by const 0
			Abc_AigPrintNode( Abc_ObjRegular(Abc_ObjFanin0(pPo)) );
			Abc_CircaNodeSetToConst0( pNtk, pPo );
			// Make sure PO is not complemented
			Abc_ObjSetFaninC( pPo, 0 );
		}
	}

	Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );

	if( !Abc_AigCheck( (Abc_Aig_t *)pNtk->pManFunc ) )
	{
		printf("precision_scaling(): Network check has failed.\n");
		return 0;
	}

	// replace the current network
	Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );

	nNodesEnd = Abc_NtkNodeNum( pNtk );
	printf("\nStarted with %d nodes. Ended with %d nodes.\n\n", nNodesStart, nNodesEnd);

	return 0;

usage:
	fprintf( pErr, "usage: precision_scaling [-h] <mask> \n" );
	// fprintf( pErr, "\t-c 	: toogles is PO is set to const-0 [default = %d]\n", fConst0 );  
	fprintf( pErr, "\t        Performs precision scaling on the circuit. \n" );
	// fprintf( pErr, "\t        qcFile is the file used for the quality check.\n" );
	fprintf( pErr, "\t        The mask is a string and each 0/1 corresponds to a PO. 1100 LSB...MSB\n" );
	fprintf( pErr, "\t        \n" );
	fprintf( pErr, "\t-h    : print the command usage.\n");
	return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


