#ifndef _SLURM_PROTOCOL_ERRNO_H
#define _SLURM_PROTOCOL_ERRNO_H

/* communcation layer RESPONSE_SLURM_RC message codes */
#define SLURM_NO_CHANGE_IN_DATA 100

/* general communication layer return codes */
#define SLURM_UNEXPECTED_MSG_ERROR 220
#define SLURM_PROTOCOL_VERSION_ERROR -100
#define SLURM_SOCKET_ERROR -1
#define SLURM_PROTOCOL_SUCCESS 0
#define SLURM_PROTOCOL_FAILURE -1

/* general return codes */
#define SLURM_SUCCESS 0
#define SLURM_ERROR -1 
#define SLURM_FAILURE -1

/* job_mgr.c/job_create */
#define ESLURM_INVALID_PARTITION_SPECIFIED	-2000
#define ESLURM_DEFAULT_PATITION_NOT_SET		-2001
#define ESLURM_JOB_MISSING_PARTITION_KEY	-2002
#define ESLURM_JOB_MISSING_REQUIRED_PARTITION_GROUP	-2003
#define ESLURM_REQUESTED_NODES_NOT_IN_PARTITION		-2004
#define ESLURM_TOO_MANY_REQUESTED_CPUS			-2005
#define ESLURM_TOO_MANY_REQUESTED_NODES			-2006
#define ESLURM_ERROR_ON_DESC_TO_RECORD_COPY		-2007

/* partition_mgr.c/update_part */
#define ESLURM_PROTOCOL_INVALID_PARTITION_NAME		-2101
#endif
