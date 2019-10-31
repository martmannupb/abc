/**CFile****************************************************************

	FileName    [AbcCircaPropertyCheck.c]

	SystemName  [ABC: Logic synthesis and verification system.]

	PackageName []

	Synopsis    []

	Author      [Linus Witschen]
	
	Affiliation [Paderborn University]

	Date        [Ver. 1.0. Started - January 25, 2017.]

	Revision    [$Id: .c,v 1.00 2017/01/25 00:00:00 alanmi Exp $]

***********************************************************************/

#include "circa/circa.h"
#include "aig/aig/aig.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static char* 	PREFIX_QC_INPUT = "qc_";

Abc_Ntk_t*      Abc_CircaOrAllPos( Abc_Ntk_t* pNtk, int fComplPos );
Abc_Ntk_t*      Abc_CircaOrAllPosInt( Abc_Ntk_t* pNtk, int fComplPos );
Abc_Ntk_t*      Abc_CircaAndAllPos( Abc_Ntk_t* pNtk, int fComplPos );
Abc_Ntk_t*      Abc_CircaAndAllPosInt( Abc_Ntk_t* pNtk, int fComplPos );
Abc_Ntk_t* 		Abc_CircaBuildPropertyChecker( Abc_Ntk_t* pNtkOrig, Abc_Ntk_t* pNtkTest, Abc_Ntk_t* pNtkQc );
Abc_Ntk_t* 		Abc_CircaBuildPropertyCheckerInt( Abc_Ntk_t* pNtkOrig, Abc_Ntk_t* pNtkTest, Abc_Ntk_t* pNtkQc );
Abc_Ntk_t* 		Abc_CircaPropertyCheckerPrepare( Abc_Ntk_t* pNtkOrig, Abc_Ntk_t* pNtkQc );
Abc_Ntk_t* 		Abc_CircaPropertyCheckerPrepareInt( Abc_Ntk_t* pNtkOrig, Abc_Ntk_t* pNtkQc );
void 			Abc_CircaPropertyCheckerAddInputs( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkPc, int fComb, char* prefix );
void            Abc_CircaPropertyCheckAddNtk( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkPc );
void 			Abc_CircaPropertyCheckConnectFanin02ObjFanout( Vec_Ptr_t* vPairs );
void 			Abc_CircaPropertyCheckAddOutputs( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkPc );
Abc_Ntk_t* 		Abc_CircaPropertyCheckerInsertNtk( Abc_Ntk_t* pNtk, Abc_Ntk_t* pNtkPcTemplate );
Abc_Ntk_t* 		Abc_CircaPropertyCheckerInsertNtkInt( Abc_Ntk_t* pNtk, Abc_Ntk_t* pNtkPcTemplate );
void 			Abc_CircaPropertyCheckerSetInputs( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkPc, int fComb );
int 			Abc_CircaEnoughEffort( int i, int nCritPaths, int changes, Circa_EffortType_t effortLvl );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

    Synopsis    []

    Description [The function takes all outputs of a circuit and ORs them.]
                             
    SideEffects []

    SeeAlso     []

***********************************************************************/
Abc_Ntk_t* Abc_CircaOrAllPos( Abc_Ntk_t* pNtk, int fComplPos )
{
    Abc_Ntk_t* pNtkOred = NULL;

    // make sure the circuits are strashed
    assert( Abc_NtkIsStrash(pNtk) );
    if( pNtk )
        pNtkOred = Abc_CircaOrAllPosInt( pNtk, fComplPos );

    return pNtkOred;
}

/**Function*************************************************************

    Synopsis    []

    Description [The function takes all outputs of a circuit and ORs them.
                 Assumes network to be combinational.]
                             
    SideEffects []

    SeeAlso     []

***********************************************************************/
Abc_Ntk_t* _Abc_CircaOrAllPosInt( Abc_Ntk_t* pNtk, int fComplPos )
{
    Abc_Ntk_t* pNtkOred = NULL;
    Abc_Obj_t* pObj, * pMiter;
    Vec_Ptr_t* vPairs = Vec_PtrAlloc( 6 );
    char Buffer[100];
    int i;

    // Start network
    pNtkOred = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    sprintf( Buffer, "%s_ored", Abc_NtkName(pNtk) );
    pNtkOred->pName = Extra_UtilStrsav(Buffer);

    // Add inputs of the circuit
    Abc_CircaPropertyCheckerAddInputs( pNtk, pNtkOred, 1, "" );
    // Add network of the circuit
    Abc_CircaPropertyCheckAddNtk( pNtk, pNtkOred );

    // collect the PO nodes for the miter
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        if( fComplPos )
            Vec_PtrPush( vPairs, Abc_ObjNot(Abc_ObjChild0Copy(pObj)) );
        else
            Vec_PtrPush( vPairs, Abc_ObjChild0Copy(pObj) );
    }
    // Miter circuit expects even number, thus, add const0 as neutral element if number is uneven
    if( (Vec_PtrSize( vPairs ) % 2) != 0 )
    {
        Abc_Obj_t* pOr = Abc_AigOr( (Abc_Aig_t *)pNtkOred->pManFunc, Abc_ObjNot(Abc_AigConst1(pNtkOred)), Abc_ObjNot(Abc_AigConst1(pNtkOred)));
        Vec_PtrPush( vPairs, pOr );
    }
    // add the miter
    pMiter = Abc_AigMiter( pNtkOred->pManFunc, vPairs, 0 );
    // Make new "valid" PO
    pObj = Abc_NtkCreatePo( pNtkOred );
    Abc_ObjAssignName( pObj, "or_out", NULL );
    // Connect new PO and miter
    Abc_ObjAddFanin( Abc_NtkPo(pNtkOred,0), pMiter );

    Vec_PtrFree( vPairs );

    return pNtkOred;
}

/**Function*************************************************************

    Synopsis    []

    Description [The function takes all outputs of a circuit and ORs them.
                 Assumes network to be combinational.]
                             
    SideEffects []

    SeeAlso     []

***********************************************************************/
Abc_Ntk_t* Abc_CircaOrAllPosInt( Abc_Ntk_t* pNtk, int fComplPos )
{
    Abc_Ntk_t* pNtkOred = NULL;
    Abc_Obj_t* pObj, * pMiter;
    Vec_Ptr_t* vPairs = Vec_PtrAlloc( 6 );
    char Buffer[100];
    int i;

    // Start network
    pNtkOred = Abc_NtkDup( pNtk );
    // sprintf( Buffer, "%s_ored", Abc_NtkName(pNtk) );
    // pNtkOred->pName = Extra_UtilStrsav(Buffer);

    // Add inputs of the circuit
    // Abc_CircaPropertyCheckerAddInputs( pNtk, pNtkOred, 1, "" );
    // Add network of the circuit
    // Abc_CircaPropertyCheckAddNtk( pNtk, pNtkOred );

    // collect the PO nodes for the miter
    Vec_Ptr_t* vOldPos = Vec_PtrAlloc( Abc_NtkPoNum( pNtk ) );
    Abc_NtkForEachPo( pNtkOred, pObj, i )
    {
        // Remember old PO
        Vec_PtrPush( vOldPos, pObj );
        if( fComplPos )
            Vec_PtrPush( vPairs, Abc_ObjNot(Abc_ObjChild0(pObj)) );
        else
            Vec_PtrPush( vPairs, Abc_ObjChild0(pObj) );
    }
    
    // Delete old Pos
    Vec_PtrForEachEntry( Abc_Obj_t*, vOldPos, pObj, i )
    {
        // printf("Removed %s %d\n",Abc_ObjName(pObj),Abc_ObjType(pObj));
        Abc_NtkDeleteObj( pObj );
    }

    // Miter circuit expects even number, thus, add const0 as neutral element if number is uneven
    if( (Vec_PtrSize( vPairs ) % 2) != 0 )
    {
        Abc_Obj_t* pOr = Abc_AigOr( (Abc_Aig_t *)pNtkOred->pManFunc, Abc_ObjNot(Abc_AigConst1(pNtkOred)), Abc_ObjNot(Abc_AigConst1(pNtkOred)));
        Vec_PtrPush( vPairs, pOr );
    }
    // add the miter
    pMiter = Abc_AigMiter( pNtkOred->pManFunc, vPairs, 0 );
    // Make new "valid" PO
    pObj = Abc_NtkCreatePo( pNtkOred );
    Abc_ObjAssignName( pObj, "or_out", NULL );
    // Connect new PO and miter
    Abc_ObjAddFanin( Abc_NtkPo(pNtkOred,0), pMiter );

    Vec_PtrFree( vPairs );

    return pNtkOred;
}

/**Function*************************************************************

    Synopsis    []

    Description [The function takes all outputs of a circuit and ANDs them.]
                             
    SideEffects []

    SeeAlso     []

***********************************************************************/
Abc_Ntk_t* Abc_CircaAndAllPos( Abc_Ntk_t* pNtk, int fComplPos )
{
    Abc_Ntk_t* pNtkAnded = NULL;

    // make sure the circuits are strashed 
    assert( Abc_NtkIsStrash(pNtk) );
    if( pNtk )
        pNtkAnded = Abc_CircaAndAllPosInt( pNtk, fComplPos );

    return pNtkAnded;
}

/**Function*************************************************************

    Synopsis    []

    Description [The function takes all outputs of a circuit and ANDs them.
                 Assumes network to be combinational.]
                             
    SideEffects []

    SeeAlso     []

***********************************************************************/
Abc_Ntk_t* Abc_CircaAndAllPosInt( Abc_Ntk_t* pNtk, int fComplPos )
{
    Abc_Ntk_t* pNtkAnded = NULL;
    Abc_Obj_t* pObj, * pMiter;
    Vec_Ptr_t* vPairs = Vec_PtrAlloc( 6 );
    char Buffer[100];
    int i;
    // Start network
    pNtkAnded = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    sprintf( Buffer, "%s_anded", Abc_NtkName(pNtk) );
    pNtkAnded->pName = Extra_UtilStrsav(Buffer);

    // Add inputs of the circuit
    Abc_CircaPropertyCheckerAddInputs( pNtk, pNtkAnded, 1, "" );
    // Add network of the circuit
    Abc_CircaPropertyCheckAddNtk( pNtk, pNtkAnded );

    // collect the PO nodes for the miter
    Abc_NtkForEachPo( pNtk, pObj, i )
    {
        Abc_Obj_t* pNode = Abc_ObjChild0Copy(pObj);
        if( fComplPos )
            Vec_PtrPush( vPairs, Abc_ObjNot(pNode) );
        else
            Vec_PtrPush( vPairs, pNode );
    }

    // Build AND tree
    Vec_Ptr_t* vNext = Vec_PtrAlloc( 3 );
    int fContinue = 1;
    while( fContinue )
    {
        // for(i=0; i < 5; i++ )
        while( Vec_PtrSize( vPairs ) )
        {
            // Pop first literal
            Abc_Obj_t* p0 = Vec_PtrPop(vPairs); 
            Abc_Obj_t* p1 = NULL; 
            // If available pop second
            if( Vec_PtrSize( vPairs ) )
            {
                p1 = Vec_PtrPop(vPairs);
                Abc_Obj_t* pAnd;
                pAnd = Abc_AigAnd( (Abc_Aig_t *)pNtkAnded->pManFunc, p0, p1);
                // Push new AND
                Vec_PtrPush( vNext, pAnd );
            }
            // If not available push last object
            else
                Vec_PtrPush( vNext, p0 );
        }
        // Assign vNext to vPairs
        if( Vec_PtrSize(vNext) > 1 )
        {
            // Free old vector
            Vec_PtrFree( vPairs );
            vPairs = Vec_PtrDup( vNext );
            // Clear next pointer
            Vec_PtrClear( vNext );
        }
        else
        {
            fContinue = 0;
        }
    }

    // add the miter
    // pMiter = Abc_AigMiter( pNtkAnded->pManFunc, vPairs, 0 );
    // Big AND output is in vNext
    pMiter = Vec_PtrPop( vNext );
    // Make new "valid" PO
    pObj = Abc_NtkCreatePo( pNtkAnded );
    Abc_ObjAssignName( pObj, "and_out", NULL );
    // Connect new PO and miter
    Abc_ObjAddFanin( Abc_NtkPo(pNtkAnded,0), pMiter );
    // Abc_ObjSetFaninC( pObj, 0 );

    Vec_PtrFree( vPairs );
    Vec_PtrFree( vNext );

    return pNtkAnded;
}

/**Function*************************************************************

	Synopsis    [Prepares the property checker circuit, i.e. connect 
				orig. circuit and quality function circuit. The app. circuit
				part will not be inserted yet.]

	Description []
							 
	SideEffects []

	SeeAlso     []

***********************************************************************/
Abc_Ntk_t* Abc_CircaBuildPropertyChecker( Abc_Ntk_t* pNtkOrig, Abc_Ntk_t* pNtkTest, Abc_Ntk_t* pNtkQc )
{
	Abc_Ntk_t* pNtkPc = NULL;

    assert( Abc_NtkHasOnlyLatchBoxes(pNtkOrig) );
    assert( Abc_NtkHasOnlyLatchBoxes(pNtkTest) );
    assert( Abc_NtkHasOnlyLatchBoxes(pNtkQc) );
    // make sure the circuits are strashed 
    assert( Abc_NtkIsStrash(pNtkOrig) );
    assert( Abc_NtkIsStrash(pNtkTest) );
    assert( Abc_NtkIsStrash(pNtkQc) );
    if ( pNtkOrig && pNtkTest && pNtkQc )
        pNtkPc = Abc_CircaBuildPropertyCheckerInt( pNtkOrig, pNtkTest, pNtkQc );

	return pNtkPc;
}


/**Function*************************************************************

  Synopsis    [Creates the Property Checker circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t* Abc_CircaBuildPropertyCheckerInt( Abc_Ntk_t* pNtkOrig, Abc_Ntk_t* pNtkTest, Abc_Ntk_t* pNtkQc )
{
	char Buffer[1000];
    Abc_Ntk_t * pNtkPc;
    Abc_Obj_t* pCi;
    int i;

    assert( Abc_NtkIsStrash(pNtkOrig) );
    assert( Abc_NtkIsStrash(pNtkTest) );
    assert( Abc_NtkIsStrash(pNtkQc) );
    // start the new network
    pNtkPc = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    sprintf( Buffer, "property_checker" );
    pNtkPc->pName = Extra_UtilStrsav(Buffer);

    // Add original network
	// Add inputs of the orig. circuit
    Abc_CircaPropertyCheckerAddInputs( pNtkOrig, pNtkPc, 1, "" );
    // Add network of the orig. circuit
    Abc_CircaPropertyCheckAddNtk( pNtkOrig, pNtkPc );

    // Add test network
    // Connect inputs to existing ones
    Abc_CircaPropertyCheckerSetInputs( pNtkTest, pNtkPc, 1 );
    // If networks are equal, the nodes won't be added as they already exist in the HASH-table
    // Only differences are added!
    Abc_CircaPropertyCheckAddNtk( pNtkTest, pNtkPc );
    // Append QC network
    // Set copy pointers of each PI to the driving node of the corresponding PO in the test or original circuit, respectively.
    // First half of PIs: Originl circuit
    for( i = 0; (i < Abc_NtkCiNum(pNtkQc)/2) && (((pCi) = Abc_NtkCi(pNtkQc, i)), 1); i++ )
        pCi->pCopy = Abc_ObjChild0Copy( Abc_NtkPo( pNtkOrig, i ) );
    // Second half of PIs: Test circuit
    for(; (i < Abc_NtkCiNum(pNtkQc)) && (((pCi) = Abc_NtkCi(pNtkQc, i)), 1); i++ )
        pCi->pCopy = Abc_ObjChild0Copy( Abc_NtkPo( pNtkTest, i-Abc_NtkCiNum(pNtkQc)/2 ) );

    // Add network of the orig. circuit
    Abc_CircaPropertyCheckAddNtk( pNtkQc, pNtkPc );
    // Add outputs of the original circuit
    Abc_CircaPropertyCheckAddOutputs( pNtkQc, pNtkPc );

    // Make sure network is in topological order
    assert( Abc_NtkIsDfsOrdered( pNtkPc ) );

    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkPc ) )
    {
        printf( "Abc_CircaBuildPropertyCheckerInt: The network check has failed.\n" );
        Abc_NtkDelete( pNtkPc );
        return NULL;
    }
    return pNtkPc;
}


/**Function*************************************************************

	Synopsis    [Prepares the property checker circuit, i.e. connect 
				orig. circuit and quality function circuit. The app. circuit
				part will not be inserted yet.]

	Description []
							 
	SideEffects []

	SeeAlso     []

***********************************************************************/
Abc_Ntk_t* Abc_CircaPropertyCheckerPrepare( Abc_Ntk_t* pNtkOrig, Abc_Ntk_t* pNtkQc )
{
	Abc_Ntk_t* pNtkPc = NULL;

    assert( Abc_NtkHasOnlyLatchBoxes(pNtkOrig) );
    assert( Abc_NtkHasOnlyLatchBoxes(pNtkQc) );
    // make sure the circuits are strashed 
    assert( Abc_NtkIsStrash(pNtkOrig) );
    assert( Abc_NtkIsStrash(pNtkQc) );

    if ( pNtkOrig && pNtkQc )
        pNtkPc = Abc_CircaPropertyCheckerPrepareInt( pNtkOrig, pNtkQc );

	return pNtkPc;
}


/**Function*************************************************************

  Synopsis    [Creates the Property Checker circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t* Abc_CircaPropertyCheckerPrepareInt( Abc_Ntk_t* pNtkOrig, Abc_Ntk_t* pNtkQc )
{
	char Buffer[1000];
    Abc_Ntk_t * pNtkPc;
    Abc_Obj_t* pObj;
    Vec_Ptr_t* vPairs = Vec_PtrAlloc( 200 );
    int i;

    assert( Abc_NtkIsStrash(pNtkOrig) );
    assert( Abc_NtkIsStrash(pNtkQc) );

    // start the new network
    pNtkPc = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    sprintf( Buffer, "property_checker_template" );
    pNtkPc->pName = Extra_UtilStrsav(Buffer);

	// Add inputs of the orig. circuit
	Abc_CircaPropertyCheckerAddInputs( pNtkOrig, pNtkPc, 1, "" );
	// Add network of the orig. circuit
	Abc_CircaPropertyCheckAddNtk( pNtkOrig, pNtkPc );
	// Add outputs of the original circuit
    Abc_CircaPropertyCheckAddOutputs( pNtkOrig, pNtkPc );

	// Add inputs of the QC circuit
	Abc_CircaPropertyCheckerAddInputs( pNtkQc, pNtkPc, 1, PREFIX_QC_INPUT );
	// Add network of the QC circuit
	Abc_CircaPropertyCheckAddNtk( pNtkQc, pNtkPc );

    // Prepare PI-PO pairs that are connected
    Abc_NtkForEachCo( pNtkOrig, pObj, i ) 
    {
    	// Add PO. Copy points to PC network
    	Vec_PtrPush( vPairs, pObj->pCopy );
    	// Add PI. Copy points to PC network
    	Vec_PtrPush( vPairs, Abc_NtkCi( pNtkQc, i )->pCopy );
    }
	Abc_CircaPropertyCheckConnectFanin02ObjFanout( vPairs );
	// Add outputs of the QC circuit
    Abc_CircaPropertyCheckAddOutputs( pNtkQc, pNtkPc );

    Vec_PtrFree( vPairs );

    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkPc ) )
    {
        printf( "Abc_CircaPropertyCheckPrepareInt: The network check has failed.\n" );
        Abc_NtkDelete( pNtkPc );
        return NULL;
    }
    return pNtkPc;
}

/**Function*************************************************************

  Synopsis    [Prepares the network for property checking.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_CircaPropertyCheckerAddInputs( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkPc, int fComb, char* prefix )
{
    Abc_Obj_t * pObj, * pObjNew;
    int i;
	char buffer[100];

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkPc);

    if ( fComb )
    {
        // create new PIs and remember them in the old PIs
        Abc_NtkForEachCi( pNtk, pObj, i )
        {
            pObjNew = Abc_NtkCreatePi( pNtkPc );
            // remember this PI in the old PIs
            pObj->pCopy = pObjNew;
            // add name
            sprintf( buffer, "%s%s", prefix, Abc_ObjName(pObj) );
            Abc_ObjAssignName( pObjNew, buffer, NULL );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Adds the network to the property checker.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_CircaPropertyCheckAddNtk( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkPc )
{
    Abc_Obj_t * pNode;
    int i;
    assert( Abc_NtkIsDfsOrdered(pNtk) );
    assert( Abc_NtkIsDfsOrdered(pNtkPc) );
    Abc_AigForEachAnd( pNtk, pNode, i )
        pNode->pCopy = Abc_AigAnd( pNtkPc->pManFunc, Abc_ObjChild0Copy(pNode), Abc_ObjChild1Copy(pNode) );
}

/**Function*************************************************************

  Synopsis    [Connects the fanin0 node to the nodes in the fanout. 
  				vPair( odd ): Node, which fanin0 fanout is re-directed, e.g. this is a PO. Output node.
  				vPair( even ): Node, which fanout nodes get a new fanin, e.g. this is a PI. Input node]

  Description []
               
  SideEffects [Problems with patching const1. May result in errors afterwards.]

  SeeAlso     []

***********************************************************************/
void Abc_CircaPropertyCheckConnectFanin02ObjFanout( Vec_Ptr_t* vPairs )
{
    Abc_Obj_t * pFanout, * pCo, * pCi, * pDriver;
    int i;

    i = 0;
    while( i < Vec_PtrSize(vPairs) )
    {
    	// Get PO
		pCo = Vec_PtrEntry( vPairs, i );
    	i++;
    	// Get PI
    	pCi = Vec_PtrEntry( vPairs, i );
    	i++;
    	// Driving node is always at position 0
		pDriver = Abc_ObjFanin0( pCo );

		// Take first element until none is left
    	while( Abc_ObjFanoutNum(pCi) )
    	{
    		pFanout = Abc_ObjFanout(pCi, 0);
			Abc_ObjPatchFanin( Abc_ObjRegular(pFanout), Abc_ObjRegular(pCi), pDriver );

    		if( Abc_ObjFanin0( Abc_ObjRegular(pFanout) ) == pDriver )
    			pFanout->fCompl0 = Abc_ObjFaninC0( pFanout ) ^ Abc_ObjFaninC0( pCo );
    		else
    			pFanout->fCompl1 = Abc_ObjFaninC1( pFanout ) ^ Abc_ObjFaninC0( pCo );
    	}
		//  Delete CI & CO copy
		Abc_NtkDeleteObj( pCi );
		Abc_NtkDeleteObj( pCo );
    }
}

/**Function*************************************************************

  Synopsis    [Adds an PO to the property checker circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_CircaPropertyCheckAddOutputs( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkPc )
{
	Abc_Obj_t* pPo, * pPoNew, * pDriver;
	int i;

    Abc_NtkForEachPo( pNtk, pPo, i )
    {
        pPoNew = Abc_NtkCreatePo( pNtkPc );
        // remember this PI in the old PIs
        pPo->pCopy = pPoNew;
        // add name
        Abc_ObjAssignName( pPoNew, Abc_ObjName(pPo), NULL );

        if( Abc_ObjChild0Copy( pPo ) )
        {
            pDriver = Abc_ObjChild0Copy( pPo );
            if( Abc_ObjRegular(pDriver) )
            {
                Abc_ObjAddFanin( Abc_ObjRegular(pPoNew), Abc_ObjRegular(pDriver) );
                if( Abc_ObjFaninC0( pPo ) )
                    Abc_ObjSetFaninC( pPoNew, 0 );
            }
            // If no driver exist tie it to const0
            else
                Abc_ObjAddFanin( Abc_ObjRegular(pPoNew), Abc_ObjNot(Abc_AigConst1( pNtkPc )) );
        }
    }
}

/**Function*************************************************************

	Synopsis    [Inserts the app. network in the already prepared
				property checker. Placeholder PIs in the QC part are replaced.]

	Description []
							 
	SideEffects [Topological order may be violated in resulting network. Better use "Abc_CircaBuildPropertyChecker"]

	SeeAlso     []

***********************************************************************/
Abc_Ntk_t* Abc_CircaPropertyCheckerInsertNtk( Abc_Ntk_t* pNtk, Abc_Ntk_t* pNtkPcTemplate )
{
	Abc_Ntk_t* pNtkPc = NULL;

    assert( Abc_NtkHasOnlyLatchBoxes(pNtk) );
    // make sure the circuits are strashed 
    assert( Abc_NtkIsStrash(pNtkPcTemplate) );
    assert( Abc_NtkIsStrash(pNtk) );

    if ( pNtkPcTemplate && pNtk )
        pNtkPc = Abc_CircaPropertyCheckerInsertNtkInt( pNtk, pNtkPcTemplate );

    // make sure that everything is okay
    Abc_AigCleanup( (Abc_Aig_t *)pNtkPc->pManFunc );
    if ( !Abc_NtkCheck( pNtkPc ) )
    {
        printf( "Abc_CircaPropertyCheckerInsertNtk: The network check has failed.\n" );
        Abc_NtkDelete( pNtkPc );
        return NULL;
    }

	return pNtkPc;
}


/**Function*************************************************************

  Synopsis    [Creates the Property Checker circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t* Abc_CircaPropertyCheckerInsertNtkInt( Abc_Ntk_t* pNtk, Abc_Ntk_t* pNtkPcTemplate )
{
	char Buffer[1000];
    Abc_Ntk_t * pNtkPc;
    Abc_Obj_t* pObj;
    Vec_Ptr_t* vPairs = Vec_PtrAlloc( 200 );
    int i;

    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkIsStrash(pNtkPcTemplate) );

    // start the new network
    pNtkPc = Abc_NtkDup( pNtkPcTemplate );
    sprintf( Buffer, "property_checker" );
    pNtkPc->pName = Extra_UtilStrsav(Buffer);

	Abc_CircaPropertyCheckerSetInputs( pNtk, pNtkPc, 1 );
	// If networks are equal, the nodes won't be added as they already exist in the HASH-table
	// Only differences are added!
	Abc_CircaPropertyCheckAddNtk( pNtk, pNtkPc );
    Abc_CircaPropertyCheckAddOutputs( pNtk, pNtkPc );
	Abc_NtkForEachCo( pNtk, pObj, i ) 
    {
    	// Add PO. Copy points to PC network
    	Vec_PtrPush( vPairs, pObj->pCopy );
    	// Add PI. Copy points to PC network
    	Vec_PtrPush( vPairs, Abc_NtkCi( pNtkPc, i+Abc_NtkCiNum(pNtkPc)-Abc_NtkCoNum(pNtk) ) );
    }
	Abc_CircaPropertyCheckConnectFanin02ObjFanout( vPairs );
	Vec_PtrFree( vPairs );

	Abc_AigCleanup( (Abc_Aig_t *)pNtkPc->pManFunc );

    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkPc ) )
    {
        printf( "Abc_CircaPropertyCheckerInsertNtkInt: The network check has failed.\n" );
        Abc_NtkDelete( pNtkPc );
        return NULL;
    }
    return pNtkPc;
}

/**Function*************************************************************

  Synopsis    [Connects the inputs of the property checker - they already exist -
  		      to the app./new circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_CircaPropertyCheckerSetInputs( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkPc, int fComb )
{
    Abc_Obj_t * pObj, * pPi;
    int i;

    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkPc);

    if ( fComb )
    {
        Abc_NtkForEachCi( pNtk, pObj, i )
        {
            pPi = Abc_NtkCi(pNtkPc, i);
            // remember this PI in the old PIs
            pObj->pCopy = pPi;
        }
    }
}

/**Function*************************************************************

	Synopsis    [Determines if enough has been spend on the circuit.]

	Description []
							 
	SideEffects []

	SeeAlso     []

***********************************************************************/
int Abc_CircaEnoughEffort( int i, int nCritPaths, int changes, Circa_EffortType_t effortLvl )
{
	if( effortLvl == CIRCA_EFFORT_MIN )
	{
		if( i == 1 )
			return 1;
		else
			return 0;
	}
	else if( effortLvl == CIRCA_EFFORT_MED )
	{
		if( i < nCritPaths )
			return 0;
		else
			return 1;
	}
	else if( effortLvl == CIRCA_EFFORT_MAX )
	{
		if( (i == nCritPaths) && !changes )
			return 1;
		else
			return 0;
	}
	return -1;
}