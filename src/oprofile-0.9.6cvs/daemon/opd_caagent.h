/**
 * @file opd_agent.h
 * Handle information received from agent.
 *
 * @remark Read the file COPYING
 *
 * @author Jason Yeh
 */

#ifndef OPD_AGENT_H
#define OPD_AGENT_H

#include "op_types.h"
#include "op_list.h"
#include "op_config.h"
#include "opd_cookie.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "opd_trans.h"

#define CA_JIT_DIR	OP_SESSION_DIR_DEFAULT "jit"


enum {
	CA_JIT_ADD, /**< nr. of JIT entries received from agent */
	CA_JIT_DEL, /**< nr. of JIT entries received from agent */
	CA_JIT_SAMPLE, /**< nr. of JIT samples mapped */
	CA_JIT_MAX_STATS
};

struct ca_jit_mapping {
	/** start of the mapping */
	vma_t start;
	/** end of the mapping */
	vma_t end;
	/** tgid of the app */
	pid_t tgid;
	/** cookie of the app */
	cookie_t app_cookie;
	/** hash list */
	struct list_head list;
	///todo see if I need to us lru_list
	/** lru list */
	//struct list_head lru_list;
	vma_t elf_header_size;
};


void ca_jit_init(void);

void ca_jit_free(void);

struct ca_jit_mapping * find_ca_jit_mapping(struct transient *);

void ca_print_jit_stats();


#ifdef __cplusplus
}
#endif

#endif /* OPD_AGENT_H */
