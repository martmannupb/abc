/**CFile****************************************************************

  FileName    [circa.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName []

  Synopsis    []

  Author      [Linus Witschen]
  
  Affiliation [Paderborn University]

  Date        [Ver. 1.0. Started - January 25, 2017.]

  Revision    [$Id: .c,v 1.00 2017/01/25 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef __CIRCA_H__
#define __CIRCA_H__

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "base/io/ioAbc.h"
#include "map/fpga/fpga.h"
#include "map/if/if.h"
#include "opt/fxu/fxu.h"
#include "opt/cut/cut.h"
#include "opt/lpk/lpk.h"
#include "opt/res/res.h"
#include "proof/fraig/fraig.h"
#include "aig/aig/aig.h"
// #include "dar.h"
#include "misc/vec/vecPtr.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// effort types
typedef enum { 
  CIRCA_EFFORT_MIN = 0, // 0:  Take only one crit. path
  CIRCA_EFFORT_MED,     // 1:  Go through all crit. paths
  CIRCA_EFFORT_MAX,     // 2:  Go through all crit. paths until nothing changes anymore
  CIRCA_EFFORT_OTHER,   // unused
} Circa_EffortType_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////


/*=== abcShow.c ==========================================================*/
extern void             Abc_NtkShow( Abc_Ntk_t * pNtk0, int fGateNames, int fSeq, int fUseReverse );

/*=== AbcCircaNtk.c ==========================================================*/
extern Abc_Ntk_t *      Abc_CircaNtkCreateCrit( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, char * pNodeName );
extern Vec_Ptr_t *      Abc_NtkDfsNodesWithMarkA( Abc_Ntk_t * pNtk, Abc_Obj_t ** ppNodes, int nNodes );
extern void             Abc_CircaNtkPrintCritNodes( FILE * pFile, Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *      Abc_CircaNtkDupMarks( Abc_Ntk_t * pNtk );
extern Abc_Ntk_t *      Abc_CircaNtkMarkNodesAndDup( Abc_Ntk_t* pNtk, Vec_Ptr_t* vNodes );
extern Abc_Obj_t *      Abc_CircaNtkFindNextMarkA( Abc_Ntk_t * pNtk );
extern void             Abc_CircaNtkMarkCrit( Abc_Ntk_t * pNtk );
extern void             Abc_CircaNtkUnmarkCrit( Abc_Ntk_t * pNtk );
/*=== AbcCircaObj.c ==========================================================*/
extern int              Abc_CircaNodeReplaceByConst0( Abc_Ntk_t * pNtk, Abc_Obj_t* pNode );
extern int              Abc_CircaNodeSetToConst0( Abc_Ntk_t * pNtk, Abc_Obj_t* pNode );
extern Vec_Ptr_t *      Abc_CircaNodeCollectCrit( Abc_Ntk_t* pNtk );
extern Vec_Ptr_t *      Abc_CircaNodeCuts( Abc_Ntk_t * pNtk, Vec_Ptr_t* vNodes, int nNodeSizeMax, int nConeSizeMax );
extern Vec_Ptr_t *      Abc_CircaNodeCutSize( Abc_Ntk_t * pNtk, Abc_Obj_t * pNode, int nNodeSizeMax, int nConeSizeMax );
extern int              Abc_CircaNodeCompareLevelsDecrease( Abc_Obj_t ** pp1, Abc_Obj_t ** pp2 );
/*=== AbcCircaPropertyCheck.c ==========================================================*/
extern Abc_Ntk_t*       Abc_CircaOrAllPos( Abc_Ntk_t* pNtk, int fComplPos );
extern Abc_Ntk_t*       Abc_CircaAndAllPos( Abc_Ntk_t* pNtk, int fComplPos );
extern Abc_Ntk_t*       Abc_CircaBuildPropertyChecker( Abc_Ntk_t* pNtkOrig, Abc_Ntk_t* pNtkTest, Abc_Ntk_t* pNtkQc );
extern Abc_Ntk_t *      Abc_CircaPropertyCheckerPrepare( Abc_Ntk_t* pNtkOrig, Abc_Ntk_t* pNtkQc );
extern Abc_Ntk_t *      Abc_CircaPropertyCheckerInsertNtk( Abc_Ntk_t* pNtk, Abc_Ntk_t* pNtkPcTemplate );
extern int              Abc_CircaEnoughEffort( int i, int nCritPaths, int changes, Circa_EffortType_t effortLvl );

/*=== circa.c ==========================================================*/
 
#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

