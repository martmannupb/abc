/**CFile****************************************************************

	FileName    [AbcCircaObj.c]

	SystemName  [ABC: Logic synthesis and verification system.]

	PackageName []

	Synopsis    []

	Author      [Linus Witschen]
	
	Affiliation [Paderborn University]

	Date        [Ver. 1.0. Started - January 25, 2017.]

	Revision    [$Id: .c,v 1.00 2017/01/25 00:00:00 alanmi Exp $]

***********************************************************************/

#include "circa/circa.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

	Synopsis    []

	Description [Given node is replaced by const0.]
							 
	SideEffects [Not needed nodes are deleted from AIG.]

	SeeAlso     []

***********************************************************************/
int Abc_CircaNodeReplaceByConst0( Abc_Ntk_t * pNtk, Abc_Obj_t* pNode )
{
	int fReplaced = 0;
	Abc_Obj_t * pConst1;

	Abc_Aig_t * pMan = (Abc_Aig_t *)pNtk->pManFunc;

	assert( Abc_NtkIsDfsOrdered(pNtk) );

	pConst1 = Abc_AigConst1( pNtk );

	char name[100];
	sprintf( name, "%s", Abc_ObjName(Abc_ObjRegular(pNode)));

	// Replace the node by const 0
	Abc_AigReplace( pMan, Abc_ObjRegular(pNode), Abc_ObjNot(pConst1), 1 );
	// Mark that node has been replaced
	fReplaced = 1;

	// Look for nodes that have only one input
	Abc_AigCleanup( pMan );

	if( !Abc_AigCheck( pMan ) )
	{
		printf("Abc_CircaReplaceNodeByConst0(): Network check has failed.\n");
		return 0;
	}

	// assert( Abc_NtkIsDfsOrdered(pNtk) );
	// printf("%s: %d\n",__func__, __LINE__);

	if( !Abc_NtkIsDfsOrdered(pNtk) )
	{
		printf("Abc_CircaNodeReplaceByConst0: Invalid graph caused by %s. AIG is not in topological order.\n", name);
		return 0;
	}

	return fReplaced;
}

/**Function*************************************************************

	Synopsis    []

	Description [Given node is set to const0 by adding a new AND which has const0 on one faninin.]
							 
	SideEffects [Not needed nodes are deleted from AIG.]

	SeeAlso     []

***********************************************************************/
int Abc_CircaNodeSetToConst0( Abc_Ntk_t * pNtk, Abc_Obj_t* pNode )
{
	int fReplaced = 0;
	Abc_Obj_t * pConst1;

	Abc_Aig_t * pMan = (Abc_Aig_t *)pNtk->pManFunc;

	assert( Abc_NtkIsDfsOrdered(pNtk) );

	pConst1 = Abc_AigConst1( pNtk );

	char name[100];
	sprintf( name, "%s", Abc_ObjName(Abc_ObjRegular(pNode)));

	// Create a new AND with driving node and const-0
	Abc_Obj_t* pAnd0 = Abc_AigAnd( pMan, Abc_ObjRegular(Abc_ObjFanin0(pNode)), Abc_ObjNot(pConst1) );
	// Disconnect Fanin
	Abc_ObjDeleteFanin( Abc_ObjRegular(pNode), Abc_ObjRegular(Abc_ObjFanin0(pNode)) );
	// Connect Fanin with newly created NAD node
	Abc_ObjAddFanin( Abc_ObjRegular(pNode), pAnd0 );

	// Mark that node has been replaced
	fReplaced = 1;

	// Look for nodes that have only one input
	Abc_AigCleanup( pMan );

	if( !Abc_AigCheck( pMan ) )
	{
		printf("Abc_CircaReplaceNodeByConst0(): Network check has failed.\n");
		return 0;
	}

	// assert( Abc_NtkIsDfsOrdered(pNtk) );
	// printf("%s: %d\n",__func__, __LINE__);

	if( !Abc_NtkIsDfsOrdered(pNtk) )
	{
		printf("Abc_CircaNodeReplaceByConst0: Invalid graph caused by %s. AIG is not in topological order.\n", name);
		return 0;
	}

	return fReplaced;
}

/**Function*************************************************************

	Synopsis    [Calculates the cut size of the node. The calling node is pushed to the end of the output vector.]

	Description []
							 
	SideEffects [Do not forget to free the pointer when done.]

	SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_CircaNodeCutSize( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, int nNodeSizeMax, int nConeSizeMax )
{
	Abc_ManCut_t * p;
	Vec_Ptr_t * vCutSmall;
	Vec_Ptr_t * vCutLarge;
	Vec_Ptr_t * vInside;
	Vec_Ptr_t * vNodesTfo;
	Vec_Ptr_t * vOut;
	Abc_Obj_t * pTmp;
	int i;
	int fIncludeFanins = 0;

	assert( Abc_NtkIsStrash(pNtk) );

	// Node is invalid
	if( Abc_ObjType(pNode) == ABC_OBJ_NONE )
		return NULL;

	// start the cut computation manager
	p = Abc_NtkManCutStart( nNodeSizeMax, nConeSizeMax, 2, ABC_INFINITY );
	// get the recovergence driven cut
	vCutSmall = Abc_NodeFindCut( p, pNode, 1 );
	// get the containing cut
	vCutLarge = Abc_NtkManCutReadCutLarge( p );
	// get the array for the inside nodes
	vInside = Abc_NtkManCutReadVisited( p );
	
	// get the inside nodes of the containing cone
	Abc_NodeConeCollect( &pNode, 1, vCutLarge, vInside, fIncludeFanins );

	// add the nodes in the TFO 
	vNodesTfo = Abc_NodeCollectTfoCands( p, pNode, vCutSmall, ABC_INFINITY );
	Vec_PtrForEachEntry( Abc_Obj_t *, vNodesTfo, pTmp, i )
	{
		Vec_PtrPushUnique( vInside, pTmp );
	}

	// Remove leafs from cone
	for(i=0; i < Vec_PtrSize(vInside); i++)
	{
		pTmp = Vec_PtrEntry(vInside, i);
		if( Abc_ObjLevel(pTmp) == 1 )
		{
			Vec_PtrRemove( vInside, pTmp );
			i--;
		}
	}

	// Store vector with cut nodes for output
	vOut = Vec_PtrDup( vInside );
	
	// Root node is stored at the, helps finding the root
	Vec_PtrPush( vOut, pNode );

	Abc_NtkManCutStop( p );

	return vOut;
}

/**Function*************************************************************

	Synopsis    []

	Description [Finds all critical paths in a network and returns its 
							roots in a vector.]
							 
	SideEffects []

	SeeAlso     []

***********************************************************************/
Vec_Ptr_t* Abc_CircaNodeCollectCrit( Abc_Ntk_t* pNtk )
{
	Abc_Obj_t* pNode, * pFanin;
	Vec_Ptr_t* vRoots = Vec_PtrAlloc( 0 );

	int i;

	// Extract root nodes of the critical paths
	Abc_NtkForEachCo( pNtk, pNode, i )
	{
		pFanin = Abc_ObjFanin0(pNode);
		if( pFanin->fMarkA == 1 )
			Vec_PtrPush( vRoots, (void *) pFanin );
	}

	return vRoots;
}

/**Function*************************************************************

	Synopsis    []

	Description [Command takes a node as parameter and determines its cut size.]
							 
	SideEffects []

	SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_CircaNodeCuts( Abc_Ntk_t * pNtk, Vec_Ptr_t* vNodes, int nNodeSizeMax, int nConeSizeMax )
{
	Abc_Obj_t * pNode;
	Vec_Ptr_t * vCuts = NULL;
	Vec_Ptr_t * vCut = NULL;
	int i;

	// Allocate some memory
	vCuts = Vec_PtrAlloc( sizeof(Vec_Ptr_t*) );

	// Take a node from vector and calc. its cut size
	Vec_PtrForEachEntryReverse( Abc_Obj_t *, vNodes, pNode, i )
	{
		// Calculate cut size for node
		if( pNode )
			vCut = Abc_CircaNodeCutSize( pNtk, pNode, nNodeSizeMax, nConeSizeMax );

		if( vCut )
		{
			// Store cut
			Vec_PtrPush( vCuts, vCut );
		}
	}

	return vCuts;
}


/**Function*************************************************************

	Synopsis    [Procedure used for sorting the nodes in decreasing order of levels.
							Can be used for Vec_PtrSort.]

	Description []
							 
	SideEffects []

	SeeAlso     []

***********************************************************************/
int Abc_CircaNodeCompareLevelsDecrease( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 )
{
	int Diff = Abc_ObjLevel(*pp1) - Abc_ObjLevel(*pp2);
	if ( Diff > 0 )
		return -1;
	if ( Diff < 0 ) 
		return 1;
	return 0; 
}