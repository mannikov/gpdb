/*-------------------------------------------------------------------------
 *
 * pg_appendonly.h
 *	  internal specifications of the appendonly relation storage.
 *
 * Portions Copyright (c) 2008-2010, Greenplum Inc.
 * Portions Copyright (c) 2012-Present VMware, Inc. or its affiliates.
 *
 *
 * IDENTIFICATION
 *	    src/include/catalog/pg_appendonly.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_APPENDONLY_H
#define PG_APPENDONLY_H

#include "catalog/genbki.h"
#include "catalog/pg_appendonly_d.h"
#include "catalog/pg_class.h"
#include "utils/relcache.h"
#include "utils/snapshot.h"

/*
 * pg_appendonly definition.
 */
CATALOG(pg_appendonly,6105,AppendOnlyRelationId)
{
	Oid				relid;				/* relation id */
    Oid             segrelid;           /* OID of aoseg table; 0 if none */
    Oid             blkdirrelid;        /* OID of aoblkdir table; 0 if none */
    Oid             blkdiridxid;        /* if aoblkdir table, OID of aoblkdir index */
	Oid             visimaprelid;		/* OID of the aovisimap table */
	Oid             visimapidxid;		/* OID of aovisimap index */
} FormData_pg_appendonly;

/* GPDB added foreign key definitions for gpcheckcat. */
FOREIGN_KEY(relid REFERENCES pg_class(oid));

/*
 * Size of fixed part of pg_appendonly tuples, not counting var-length fields
 * (there are no var-length fields currentl.)
*/
#define APPENDONLY_TUPLE_SIZE \
	 (offsetof(FormData_pg_appendonly,visimapidxid) + sizeof(Oid))

/* ----------------
*		Form_pg_appendonly corresponds to a pointer to a tuple with
*		the format of pg_appendonly relation.
* ----------------
*/
typedef FormData_pg_appendonly *Form_pg_appendonly;

/*
 * AORelationVersion defines valid values for the version of AppendOnlyEntry.
 * NOTE: When this is updated, AoRelationVersion_GetLatest() must be updated accordingly.
 */
typedef enum AORelationVersion
{
	AORelationVersion_None =  0,
	AORelationVersion_Original =  1,		/* first valid version */
	AORelationVersion_Aligned64bit = 2,		/* version where the fixes for AOBlock and MemTuple
											 * were introduced, see MPP-7251 and MPP-7372. */
	AORelationVersion_PG83 = 3,				/* Same as Aligned64bit, but numerics are stored
											 * in the PostgreSQL 8.3 format. */
	AORelationVersion_PG12 = 4,             /* version that removed block directory hole filling. */
	MaxAORelationVersion                    /* must always be last */
} AORelationVersion;

#define AORelationVersion_GetLatest() AORelationVersion_PG12

#define AORelationVersion_IsValid(version) \
	(version > AORelationVersion_None && version < MaxAORelationVersion)

extern bool Debug_appendonly_print_verify_write_block;

static inline void AORelationVersion_CheckValid(int version)
{
	if (!AORelationVersion_IsValid(version))
	{
		ereport(Debug_appendonly_print_verify_write_block?PANIC:ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("append-only table version %d is invalid", version),
				 errprintstack(true)));
	}
}

/*
 * Versions higher than AORelationVersion_Original include the fixes for AOBlock and
 * MemTuple alignment.
 */
#define IsAOBlockAndMemtupleAlignmentFixed(version) \
( \
	AORelationVersion_CheckValid(version), \
	(version > AORelationVersion_Original) \
)

extern void
InsertAppendOnlyEntry(Oid relid,
					  Oid segrelid,
					  Oid blkdirrelid,
					  Oid blkdiridxid,
					  Oid visimaprelid,
					  Oid visimapidxid);

void
GetAppendOnlyEntryAttributes(Oid relid,
							 int32 *blocksize,
							 int16 *compresslevel,
							 bool *checksum,
							 NameData *compresstype);

/*
 * Get the OIDs of the auxiliary relations and their indexes for an appendonly
 * relation.
 *
 * The OIDs will be retrieved only when the corresponding output variable is
 * not NULL.
 */
void
GetAppendOnlyEntryAuxOids(Relation rel,
						  Oid *segrelid,
						  Oid *blkdirrelid,
						  Oid *blkdiridxid,
						  Oid *visimaprelid,
						  Oid *visimapidxid);


void
GetAppendOnlyEntry(Relation rel, Form_pg_appendonly aoEntry);
/*
 * Update the segrelid and/or blkdirrelid if the input new values
 * are valid OIDs.
 */
extern void
UpdateAppendOnlyEntryAuxOids(Oid relid,
							 Oid newSegrelid,
							 Oid newBlkdirrelid,
							 Oid newBlkdiridxid,
							 Oid newVisimaprelid,
							 Oid newVisimapidxid);

extern void
RemoveAppendonlyEntry(Oid relid);

extern void ATAOEntries(Form_pg_class relform1, Form_pg_class relform2);

#endif   /* PG_APPENDONLY_H */
