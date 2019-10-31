/**CFile****************************************************************

	FileName    [AbcCircaNtk.c]

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

/*=== abcBalance.c ==========================================================*/
extern void             Abc_NtkMarkCriticalNodes( Abc_Ntk_t * pNtk );
/*=== abcDfs.c ==========================================================*/
extern void             Abc_NtkDfs_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the critical path from a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_CircaNtkCreateCrit( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, char * pNodeName )
{
    Abc_Ntk_t * pNtkNew; 
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pFanin, * pFanout, * pNodeCoNew;
    char Buffer[1000];
    int i, k;

    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );
    assert( Abc_ObjIsNode(pNode) ); 
    
    // start the network
    pNtkNew = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );
    // set the name
    sprintf( Buffer, "%s_%s", pNtk->pName, pNodeName );
    pNtkNew->pName = Extra_UtilStrsav(Buffer);

    // establish connection between the constant nodes
    if ( Abc_NtkIsStrash(pNtk) )
        Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);

    // collect the nodes in the TFI of the output
    vNodes = Abc_NtkDfsNodes( pNtk, &pNode, 1 );

    // create the PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        if ( Abc_NodeIsTravIdCurrent(pObj) ) // TravId is set by DFS
        {
            pObj->pCopy = Abc_NtkCreatePi(pNtkNew);
            Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
        }
    }
    // add the PO corresponding to this output
    pNodeCoNew = Abc_NtkCreatePo( pNtkNew );
    Abc_ObjAssignName( pNodeCoNew, pNodeName, NULL );
    // copy the nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        // if it is an AIG, add to the hash table
        if ( Abc_NtkIsStrash(pNtk) )
        {
            pObj->pCopy = Abc_AigAnd( pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
            pObj->pCopy->fMarkA = pObj->fMarkA;
        }
        else
        {
            Abc_NtkDupObj( pNtkNew, pObj, 0 );
            Abc_ObjForEachFanin( pObj, pFanin, k )
            {
                Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
            }   
        }
    }
    // connect the internal nodes to the new CO
    Abc_ObjAddFanin( pNodeCoNew, pNode->pCopy );
    Vec_PtrFree( vNodes );


    Abc_AigForEachAnd( pNtkNew, pObj, i )
    {

        if( (pObj->fMarkA == 0) ) {
            Abc_ObjForEachFanout( pObj, pFanout, k )
            {
                Abc_ObjDeleteFanin( pFanout, pObj );
            }
        }
    }
    Abc_AigForEachAnd( pNtkNew, pObj, i )
    {
        if( (pObj->fMarkA == 0) ) {
            printf("Would delete ID: %d\n", pObj->Id);
            Abc_AigDeleteNode( pNtkNew->pManFunc, pObj);
        }
    }
    Abc_NtkForEachNode( pNtkNew, pObj, i )
    {
        if( (pObj->fMarkA == 0) ) {
            printf("Would delete ID: %d\n", pObj->Id);
            Abc_NtkDeleteObj(pObj);
        }
    }
    Abc_NtkForEachCi( pNtkNew, pObj, i )
    {
        int marks = 0;
        Abc_ObjForEachFanout( pObj, pFanout, k )
        {
            if( pFanout->fMarkA == 0 )
                marks++;
        }
        if( pObj->vFanouts.nSize == marks ) {
            printf("DELETED PI: %s\n", Abc_ObjName(pObj));
            Abc_NtkDeleteObj(pObj);
        }
    }

    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkCreateCrit(): Network check has failed.\n" );

    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Returns the DFS ordered array of logic nodes.]

  Description [Collects only the internal nodes, leaving out PIs, POs and latches.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkDfsNodesWithMarkA( Abc_Ntk_t * pNtk, Abc_Obj_t ** ppNodes, int nNodes )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i;
    // set the traversal ID
    Abc_NtkIncrementTravId( pNtk );
    // start the array of nodes
    vNodes = Vec_PtrAlloc( 100 );
    // go through the PO nodes and call for each of them
    for ( i = 0; i < nNodes; i++ )
    {
        if ( Abc_ObjIsCo(ppNodes[i]) )
        {
            Abc_NodeSetTravIdCurrent(ppNodes[i]);
            Abc_NtkDfs_rec( Abc_ObjFanin0Ntk(Abc_ObjFanin0(ppNodes[i])), vNodes );
        }
        else if ( Abc_ObjIsNode(ppNodes[i]) )
            Abc_NtkDfs_rec( ppNodes[i], vNodes );
    }

    for(i=0; i < vNodes->nSize; i++)
    {
        pObj = Vec_PtrEntry(vNodes, i);
        if( pObj->fMarkA == 0 ) {
            Vec_PtrRemove( vNodes, pObj );
            // Size decrease, thus, decrease iterator, too
            i--;
        }
    }

    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Prints the nodes on the critical path by level.]

  Description []
               
  SideEffects [Very inefficient right now. ]

  SeeAlso     []

***********************************************************************/
void Abc_CircaNtkPrintCritNodes( FILE * pFile, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode, * pFanout;
    int i, k;

    int nLevels;
    nLevels = Abc_NtkLevel(pNtk);

    Vec_Ptr_t* vNodes = Vec_PtrAlloc( nLevels );

    printf( "Nodes on critical path sorted by level:\n" );

    Abc_NtkForEachPi( pNtk, pNode, i)
    {
        int fCrit = 0;
        for( k=0; (k < Abc_ObjFanoutNum( pNode )) && (!fCrit); k++ )
        {
            pFanout = Abc_ObjFanout( pNode, k );
            if( pFanout->fMarkA )
            {
                fCrit = 1;
                Vec_PtrPushUnique( vNodes, pNode );
            }
        }
    }

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        if( pNode->fMarkA )
            Vec_PtrPush( vNodes, pNode );
    }

    Abc_NtkForEachPo( pNtk, pNode, i)
    {
        if( Abc_ObjFanin0( pNode )->fMarkA )
            Vec_PtrPush( vNodes, pNode );
    }

    Vec_PtrSort( vNodes, Abc_CircaNodeCompareLevelsDecrease );

    int curLvl = 0;
    printf("Level %d:\t", curLvl);
    while( Vec_PtrSize( vNodes ) )
    {
        pNode = Vec_PtrPop( vNodes );
        if( curLvl == Abc_ObjLevel(pNode) )
        {
            printf("%s ", Abc_ObjName(pNode));
        }
        else {
            curLvl = Abc_ObjLevel( pNode );
            printf("\nLevel %d:\t%s ", curLvl, Abc_ObjName(pNode));
        }
    }
    printf("\n");

    Vec_PtrFree( vNodes );

    return;
}

/**Function*************************************************************

	Synopsis    [Duplicate the network and also copies the node's marks.]

	Description []
							 
	SideEffects []

	SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_CircaNtkDupMarks( Abc_Ntk_t * pNtk )
{
		Abc_Ntk_t * pNtkNew; 
		Abc_Obj_t * pObj, * pFanin;
		int i, k;
		if ( pNtk == NULL )
				return NULL;
		// start the network
		pNtkNew = Abc_NtkStartFrom( pNtk, pNtk->ntkType, pNtk->ntkFunc );
		// copy the internal nodes
		if ( Abc_NtkIsStrash(pNtk) )
		{
				// copy the AND gates
				Abc_AigForEachAnd( pNtk, pObj, i )
				{
						pObj->pCopy = Abc_AigAnd( pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
						pObj->pCopy->fMarkA = pObj->fMarkA;
						pObj->pCopy->fMarkB = pObj->fMarkB;
						pObj->pCopy->fMarkC = pObj->fMarkC;
				}
				// relink the choice nodes
				Abc_AigForEachAnd( pNtk, pObj, i )
						if ( pObj->pData )
								pObj->pCopy->pData = ((Abc_Obj_t *)pObj->pData)->pCopy;
				// relink the CO nodes
				Abc_NtkForEachCo( pNtk, pObj, i )
						Abc_ObjAddFanin( pObj->pCopy, Abc_ObjChild0Copy(pObj) );
				// get the number of nodes before and after
				if ( Abc_NtkNodeNum(pNtk) != Abc_NtkNodeNum(pNtkNew) )
						printf( "Warning: Structural hashing during duplication reduced %d nodes (this is a minor bug).\n",
								Abc_NtkNodeNum(pNtk) - Abc_NtkNodeNum(pNtkNew) );
		}
		else
		{
				// duplicate the nets and nodes (CIs/COs/latches already dupped)
				Abc_NtkForEachObj( pNtk, pObj, i )
						if ( pObj->pCopy == NULL )
								Abc_NtkDupObj(pNtkNew, pObj, 0);
				// reconnect all objects (no need to transfer attributes on edges)
				Abc_NtkForEachObj( pNtk, pObj, i )
						if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
								Abc_ObjForEachFanin( pObj, pFanin, k )
										Abc_ObjAddFanin( pObj->pCopy, pFanin->pCopy );
		}
		// duplicate the EXDC Ntk
		if ( pNtk->pExdc )
				pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
		if ( !Abc_NtkCheck( pNtkNew ) )
				fprintf( stdout, "Abc_NtkDup(): Network check has failed.\n" );
		pNtk->pCopy = pNtkNew;
		return pNtkNew;
}

/**Function*************************************************************

	Synopsis    [Duplicate the network and also copies the node's marks.]

	Description []
							 
	SideEffects []

	SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_CircaNtkMarkNodesAndDup( Abc_Ntk_t* pNtk, Vec_Ptr_t* vNodes )
{
	Abc_Ntk_t* pNtkDup;
	Abc_Obj_t* pNode;

	int i;

	// Mark node
	Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
			pNode->fMarkA = 1;

	// Duplicate app. circuit and mark candidate
	pNtkDup = Abc_CircaNtkDupMarks( pNtk );

	// To avoid errors unmark it again
	Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
		pNode->fMarkA = 0;

	return pNtkDup;
}

/**Function*************************************************************

	Synopsis    [Returns the first node in the network with a set MarkA.]

	Description []
							 
	SideEffects [Unmarks the node.]

	SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_CircaNtkFindNextMarkA( Abc_Ntk_t * pNtk )
{
		int i;
		int fFound = 0;
		Abc_Obj_t* pNode = NULL;
		for(i = 0; (i < Vec_PtrSize((pNtk)->vObjs)) && !fFound; ++i)
		{
				pNode = Abc_NtkObj(pNtk, i);
				if ( (pNode) == NULL || Abc_ObjIsNode(pNode) )
				{
						if( pNode->fMarkA )
						{
								fFound = 1;
								// Clean the mark
								pNode->fMarkA = 0;
						}
				}
		}

		return pNode;
}

/**Function*************************************************************

	Synopsis    [Sets Mark A for all nodes on the critical path(s).]

	Description []
							 
	SideEffects [Do not forget to unmark set marks before calling other function, because they might fail, e.g. Abc_AigReplace.]

	SeeAlso     []

***********************************************************************/
void Abc_CircaNtkMarkCrit( Abc_Ntk_t * pNtk )
{
	Abc_NtkStartReverseLevels( pNtk, 0 );
	Abc_NtkMarkCriticalNodes( pNtk );
}

/**Function*************************************************************

	Synopsis    [Unsets Mark A for all nodes in the network.]

	Description []
							 
	SideEffects []

	SeeAlso     []

***********************************************************************/
void Abc_CircaNtkUnmarkCrit( Abc_Ntk_t * pNtk )
{
	Abc_NtkCleanMarkA( pNtk );
}