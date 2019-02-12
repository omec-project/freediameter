/*********************************************************************************************************
* Software License Agreement (BSD License)                                                               *
* Author: Sebastien Decugis <sdecugis@freediameter.net>							 *
*													 *
* Copyright (c) 2015, WIDE Project and NICT								 *
* All rights reserved.											 *
* 													 *
* Redistribution and use of this software in source and binary forms, with or without modification, are  *
* permitted provided that the following conditions are met:						 *
* 													 *
* * Redistributions of source code must retain the above 						 *
*   copyright notice, this list of conditions and the 							 *
*   following disclaimer.										 *
*    													 *
* * Redistributions in binary form must reproduce the above 						 *
*   copyright notice, this list of conditions and the 							 *
*   following disclaimer in the documentation and/or other						 *
*   materials provided with the distribution.								 *
* 													 *
* * Neither the name of the WIDE Project or NICT nor the 						 *
*   names of its contributors may be used to endorse or 						 *
*   promote products derived from this software without 						 *
*   specific prior written permission of WIDE Project and 						 *
*   NICT.												 *
* 													 *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED *
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A *
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 	 *
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 	 *
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR *
* TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF   *
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.								 *
*********************************************************************************************************/

#include "fdproto-internal.h"
#include <inttypes.h>

#define ENABLE_LOCK_BYPASS 1
#define USE_HASHLIST 1

#if USE_HASHLIST
   int initInt32HashList(void **hl);
   void deleteInt32HashList(void *hl);
   void deleteEntryInt32HashList(int32_t k, void *hl);
   int insertInt32HashList(int32_t k, void *v, void *hl, void **duplicate);
   int findInt32HashList(int32_t k, void *hl, void **result);

   int initInt64HashList(void **hl);
   void deleteInt64HashList(void *hl);
   void deleteEntryInt64HashList(int64_t k, void *hl);
   int insertInt64HashList(int64_t k, void *v, void *hl, void **duplicate);
   int findInt64HashList(int64_t k, void *hl, void **result);

   int initUInt32HashList(void **hl);
   void deleteUInt32HashList(void *hl);
   void deleteEntryUInt32HashList(uint32_t k, void *hl);
   int insertUInt32HashList(uint32_t k, void *v, void *hl, void **duplicate);
   int findUInt32HashList(uint32_t k, void *hl, void **result);

   int initUInt64HashList(void **hl);
   void deleteUInt64HashList(void *hl);
   void deleteEntryUInt64HashList(uint64_t k, void *hl);
   int insertUInt64HashList(uint64_t k, void *v, void *hl, void **duplicate);
   int findUInt64HashList(uint64_t k, void *hl, void **result);

   int initFloat32HashList(void **hl);
   void deleteFloat32HashList(void *hl);
   void deleteEntryFloat32HashList(float k, void *hl);
   int insertFloat32HashList(float k, void *v, void *hl, void **duplicate);
   int findFloat32HashList(float k, void *hl, void **result);

   int initFloat64HashList(void **hl);
   void deleteFloat64HashList(void *hl);
   void deleteEntryFloat64HashList(double k, void *hl);
   int insertFloat64HashList(double k, void *v, void *hl, void **duplicate);
   int findFloat64HashList(double k, void *hl, void **result);

   int initStringHashList(void **hl);
   void deleteStringHashList(void *hl);
   void deleteEntryStringHashList(const char *k, void *hl);
   int insertStringHashList(const char *k, void *v, void *hl, void **duplicate);
   int findStringHashList(const char *k, void *hl, void **result);
#endif

/* Names of the base types */
const char * type_base_name[] = { /* must keep in sync with dict_avp_basetype */
	"GROUPED", 	/* AVP_TYPE_GROUPED */
	"OCTETSTRING", 	/* AVP_TYPE_OCTETSTRING */
	"INTEGER32", 	/* AVP_TYPE_INTEGER32 */
	"INTEGER64", 	/* AVP_TYPE_INTEGER64 */
	"UNSIGNED32", 	/* AVP_TYPE_UNSIGNED32 */
	"UNSIGNED64", 	/* AVP_TYPE_UNSIGNED64 */
	"FLOAT32", 	/* AVP_TYPE_FLOAT32 */
	"FLOAT64"	/* AVP_TYPE_FLOAT64 */
	};

/* The number of lists in an object */
#define NB_LISTS_PER_OBJ	3

/* Some eye catchers definitions */
#define OBJECT_EYECATCHER	(0x0b13c7)
#define DICT_EYECATCHER		(0x00d1c7)

/* Definition of the dictionary objects */
struct dict_object {
	enum dict_object_type	type;	/* What type of object is this? */
	int			objeyec;/* eyecatcher for this object */
	int			typeyec;/* eyecatcher for this type of object */
	struct dictionary	*dico;  /* The dictionary this object belongs to */
	
	union {
		struct dict_vendor_data		vendor;		/* datastr_len = strlen(vendor_name) */
		struct dict_application_data	application;	/* datastr_len = strlen(application_name) */
		struct dict_type_data		type;		/* datastr_len = strlen(type_name) */
		struct dict_enumval_data	enumval;	/* datastr_len = strlen(enum_name) */
		struct dict_avp_data		avp;		/* datastr_len = strlen(avp_name) */
		struct dict_cmd_data		cmd;		/* datastr_len = strlen(cmd_name) */
		struct dict_rule_data		rule;		/* datastr_len = 0 */
	} data;				/* The data of this object */
	
	size_t			datastr_len; /* cached length of the string inside the data. Saved when the object is created. */
	
	struct dict_object *	parent; /* The parent of this object, if any */
	
	struct fd_list		list[NB_LISTS_PER_OBJ];/* used to chain objects.*/
#ifdef USE_HASHLIST
	void *            hashlist[NB_LISTS_PER_OBJ];
#endif

	/* More information about the lists :
	
	 - the use for each list depends on the type of object. See detail below.
	 
	 - a sentinel for a list has its 'o' field cleared. (this is the criteria to detect end of a loop)
	 
	 - The lists are always ordered. The criteria are described below. the functions to order them are referenced in dict_obj_info
	 
	 - The dict_lock must be held for any list operation.
	 
	 => VENDORS:
	 list[0]: list of the vendors, ordered by their id. The sentinel is g_dict_vendors (vendor with id 0)
	 list[1]: sentinel for the list of AVPs from this vendor, ordered by AVP code.
	 list[2]: sentinel for the list of AVPs from this vendor, ordered by AVP name (fd_os_cmp).
	 
	 => APPLICATIONS:
	 list[0]: list of the applications, ordered by their id. The sentinel is g_dict_applications (application with id 0)
	 list[1]: not used
	 list[2]: not used.
	 
	 => TYPES:
	 list[0]: list of the types, ordered by their names. The sentinel is g_list_types.
	 list[1]: sentinel for the type_enum list of this type, ordered by their constant name (fd_os_cmp).
	 list[2]: sentinel for the type_enum list of this type, ordered by their constant value.
	 
	 => TYPE_ENUMS:
	 list[0]: list of the contants for a given type, ordered by the constant name (fd_os_cmp). Sentinel is a (list[1]) element of a TYPE object.
	 list[1]: list of the contants for a given type, ordered by the constant value. Sentinel is a (list[2]) element of a TYPE object.
	 list[2]: not used
	 
	 => AVPS:
	 list[0]: list of the AVP from a given vendor, ordered by avp code. Sentinel is a list[1] element of a VENDOR object.
	 list[1]: list of the AVP from a given vendor, ordered by avp name (fd_os_cmp). Sentinel is a list[2] element of a VENDOR object.
	 list[2]: sentinel for the rule list that apply to this AVP.
	 
	 => COMMANDS:
	 list[0]: list of the commands, ordered by their names (fd_os_cmp). The sentinel is g_list_cmd_name.
	 list[1]: list of the commands, ordered by their command code and 'R' flag. The sentinel is g_list_cmd_code.
	 list[2]: sentinel for the rule list that apply to this command.
	 
	 => RULES:
	 list[0]: list of the rules for a given (grouped) AVP or Command, ordered by the AVP vendor & code to which they refer. sentinel is list[2] of a command or (grouped) avp.
	 list[1]: not used
	 list[2]: not used.
	 
	 */
	 
	 /* Sentinel for the dispatch callbacks */
	 struct fd_list		disp_cbs;
	
};

/* Definition of the dictionary structure */
struct dictionary {
	int		 	dict_eyec;		/* Eye-catcher for the dictionary (DICT_EYECATCHER) */
	
#if ENABLE_LOCK_BYPASS
	int			dict_bypass_lock;	/* When true, don't use the dict_lock */
#endif
	pthread_rwlock_t 	dict_lock;		/* The global rwlock for the dictionary */
	
	struct dict_object	dict_vendors;		/* Sentinel for the list of vendors, corresponding to vendor 0 */
	struct dict_object	dict_applications;	/* Sentinel for the list of applications, corresponding to app 0 */
	struct fd_list		dict_types;		/* Sentinel for the list of types */
	struct fd_list		dict_cmd_name;		/* Sentinel for the list of commands, ordered by names */
	struct fd_list		dict_cmd_code;		/* Sentinel for the list of commands, ordered by codes */
	
	struct dict_object	dict_cmd_error;		/* Special command object for answers with the 'E' bit set */
	
	int			dict_count[DICT_TYPE_MAX + 1]; /* Number of objects of each type */
};

/* Forward declarations of dump functions */
static DECLARE_FD_DUMP_PROTOTYPE(dump_vendor_data, void * data );
static DECLARE_FD_DUMP_PROTOTYPE(dump_application_data, void * data );
static DECLARE_FD_DUMP_PROTOTYPE(dump_type_data, void * data );
  /* the dump function for enum has a different prototype since it need the datatype */
static DECLARE_FD_DUMP_PROTOTYPE(dump_avp_data, void * data );
static DECLARE_FD_DUMP_PROTOTYPE(dump_command_data, void * data );
static DECLARE_FD_DUMP_PROTOTYPE(dump_rule_data, void * data );

/* Forward declarations of search functions */
static int search_vendor 	( struct dictionary * dict, int criteria, const void * what, struct dict_object **result );
static int search_application   ( struct dictionary * dict, int criteria, const void * what, struct dict_object **result );
static int search_type 		( struct dictionary * dict, int criteria, const void * what, struct dict_object **result );
static int search_enumval 	( struct dictionary * dict, int criteria, const void * what, struct dict_object **result );
static int search_avp		( struct dictionary * dict, int criteria, const void * what, struct dict_object **result );
static int search_cmd		( struct dictionary * dict, int criteria, const void * what, struct dict_object **result );
static int search_rule		( struct dictionary * dict, int criteria, const void * what, struct dict_object **result );

/* The following array contains lot of data about the different types of objects, for automated handling */
static struct {
	enum dict_object_type 	type; 		/* information for this type */
	char *			name;		/* string describing this object, for debug */
	size_t			datasize;	/* The size of the data structure */
	int			parent;		/* 0: never; 1: may; 2: must */
	enum dict_object_type	parenttype;	/* The type of the parent, when relevant */
	int			eyecatcher;	/* A kind of signature for this object */
	DECLARE_FD_DUMP_PROTOTYPE( (*dump_data), void * data );	/* The function to dump the data section */
	int 		      (*search_fct)(struct dictionary * dict, int criteria, const void * what, struct dict_object **result );;	/* The function to search an object of this type */
	int			haslist[NB_LISTS_PER_OBJ];	/* Tell if this list is used */
} dict_obj_info[] = { { 0, "(error)", 0, 0, 0, 0, NULL, NULL, {0, 0, 0} }

	/* type			 name		datasize		   	  parent  	parenttype 
			eyecatcher		dump_data	  	search_fct,		haslist[] 	*/

	,{ DICT_VENDOR,		"VENDOR",	sizeof(struct dict_vendor_data),	0, 	0,
			OBJECT_EYECATCHER + 1, 	dump_vendor_data, 	search_vendor, 		{ 1, 0, 0 } }
	
	,{ DICT_APPLICATION,	"APPLICATION",	sizeof(struct dict_application_data),	1, 	DICT_VENDOR,
			OBJECT_EYECATCHER + 2,	dump_application_data,	search_application,	{ 1, 0, 0 } }
	
	,{ DICT_TYPE,		"TYPE",		sizeof(struct dict_type_data),		1, 	DICT_APPLICATION,
			OBJECT_EYECATCHER + 3,	dump_type_data,		search_type,		{ 1, 0, 0 } }
	
	,{ DICT_ENUMVAL,	"ENUMVAL",	sizeof(struct dict_enumval_data),	2, 	DICT_TYPE,
			OBJECT_EYECATCHER + 4,	NULL,			search_enumval,	{ 1, 1, 0 } }
	
	,{ DICT_AVP,		"AVP",		sizeof(struct dict_avp_data),		1, 	DICT_TYPE,
			OBJECT_EYECATCHER + 5,	dump_avp_data,		search_avp,		{ 1, 1,	0 } }
	
	,{ DICT_COMMAND,	"COMMAND",	sizeof(struct dict_cmd_data),		1, 	DICT_APPLICATION,
			OBJECT_EYECATCHER + 6,	dump_command_data,	search_cmd,		{ 1, 1, 0 } }
	
	,{ DICT_RULE,		"RULE",		sizeof(struct dict_rule_data),		2, 	-1 /* special case: grouped avp or command */,
			OBJECT_EYECATCHER + 7,	dump_rule_data,		search_rule,		{ 1, 0, 0 } }
	
};
	
/* Macro to verify a "type" value */
#define CHECK_TYPE( type ) ( ((type) > 0) && ((type) <= DICT_TYPE_MAX) )

/* Cast macro */
#define _O( object ) ((struct dict_object *) (object))

/* Get information line for a given object */
#define _OBINFO(object) (dict_obj_info[CHECK_TYPE(_O(object)->type) ? _O(object)->type : 0])




/*******************************************************************************************************/
/*******************************************************************************************************/
/*                                                                                                     */
/*                                  Objects management                                                 */
/*                                                                                                     */
/*******************************************************************************************************/
/*******************************************************************************************************/

/* Functions to manage the objects creation and destruction. */

/* Duplicate a string inplace, save its length */
#define DUP_string_len( str, plen ) {		\
	*(plen) = strlen((str));		\
	str = os0dup( str, *(plen));		\
}
	
/* Initialize an object */
static void init_object( struct dict_object * obj, enum dict_object_type type )
{
	int i;
	
	TRACE_ENTRY("%p %d", obj, type);
	
	/* Clean the object first */
	memset ( obj, 0, sizeof(struct dict_object));
	
	CHECK_PARAMS_DO(  CHECK_TYPE(type),  return  );

	obj->type = type;
	obj->objeyec = OBJECT_EYECATCHER;
	obj->typeyec = _OBINFO(obj).eyecatcher;

	/* We don't initialize the data nor the parent here */
	
	/* Now init the lists */
	for (i=0; i<NB_LISTS_PER_OBJ; i++) {
		if (_OBINFO(obj).haslist[i] != 0) 
			fd_list_init(&obj->list[i], obj);
		else
			fd_list_init(&obj->list[i], NULL);
	}
	
	fd_list_init(&obj->disp_cbs, NULL);
}

/* Initialize the "data" part of an object */
static int init_object_data(struct dict_object * dest, void * source, enum dict_object_type type, int dupos)
{
	TRACE_ENTRY("%p %p %d", dest, source, type);
	CHECK_PARAMS( dest && source && CHECK_TYPE(type) );
	
	/* Generic: copy the full data structure */	
	memcpy( &dest->data, source, dict_obj_info[type].datasize );
	
	/* Then strings must be duplicated, not copied */
	/* This function might be simplified by always defining the "name" field as the first field of the structures, but... it's error-prone */
	switch (type) {
		case DICT_VENDOR:
			DUP_string_len( dest->data.vendor.vendor_name, &dest->datastr_len );
#if USE_HASHLIST
			initUInt32HashList(&dest->hashlist[0]);
         initStringHashList(&dest->hashlist[1]);
#endif
			break;
		
		case DICT_APPLICATION:
			DUP_string_len( dest->data.application.application_name, &dest->datastr_len );
			break;
			
		case DICT_TYPE:
			DUP_string_len( dest->data.type.type_name, &dest->datastr_len );
#if USE_HASHLIST
			switch (dest->data.type.type_base)
			{
			   case AVP_TYPE_OCTETSTRING:
			      break;
            case AVP_TYPE_INTEGER32:
               initInt32HashList(&dest->hashlist[0]);
               break;
            case AVP_TYPE_INTEGER64:
               initInt64HashList(&dest->hashlist[0]);
               break;
            case AVP_TYPE_UNSIGNED32:
               initUInt32HashList(&dest->hashlist[0]);
               break;
            case AVP_TYPE_UNSIGNED64:
               initUInt64HashList(&dest->hashlist[0]);
               break;
            case AVP_TYPE_FLOAT32:
               initFloat32HashList(&dest->hashlist[0]);
               break;
            case AVP_TYPE_FLOAT64:
               initFloat64HashList(&dest->hashlist[0]);
               break;
            case AVP_TYPE_GROUPED:
            default:
               break;
			}
			initStringHashList(&dest->hashlist[1]);
#endif
			break;
			
		case DICT_ENUMVAL:
			DUP_string_len( dest->data.enumval.enum_name, &dest->datastr_len );
			if (dupos) {
				// we also need to duplicate the octetstring constant value since it is a pointer.
				dest->data.enumval.enum_value.os.data = os0dup( 
						((struct dict_enumval_data *)source)->enum_value.os.data, 
						((struct dict_enumval_data *)source)->enum_value.os.len
					);
			}
			break;

		case DICT_AVP:
			DUP_string_len( dest->data.avp.avp_name, &dest->datastr_len );
			break;
			
		case DICT_COMMAND:
			DUP_string_len( dest->data.cmd.cmd_name, &dest->datastr_len );
			break;
		
		default:
			/* Nothing to do for RULES */
			;
	}
	
	return 0;
}

/* Check that an object is valid (1: OK, 0: error) */
static int verify_object( struct dict_object * obj )
{
	TRACE_ENTRY("%p", obj);
	
	CHECK_PARAMS_DO(  obj
			&& (obj->objeyec == OBJECT_EYECATCHER)
			&& CHECK_TYPE(obj->type)
			&& (obj->typeyec == dict_obj_info[obj->type].eyecatcher),
		{
			if (obj) {
				TRACE_DEBUG(FULL, "Invalid object: %p, obj->objeyec: %x/%x, obj->type: %d, obj->objeyec: %x/%x, obj->typeyec: %x/%x",
						obj,
						obj->objeyec, OBJECT_EYECATCHER,
						obj->type,
						obj->objeyec, OBJECT_EYECATCHER,
						obj->typeyec, _OBINFO(obj).eyecatcher);
			} else {
				TRACE_DEBUG(FULL, "Invalid object : NULL pointer");
			}
			return 0;
		}  );
	
	/* The object is probably valid. */
	return 1;
}

/* Free the data associated to an object */
static void destroy_object_data(struct dict_object * obj)
{
	/* TRACE_ENTRY("%p", obj); */
	
	switch (obj->type) {
		case DICT_VENDOR:
			free( obj->data.vendor.vendor_name );
#if USE_HASHLIST
			deleteUInt32HashList(obj->hashlist[0]);
         deleteStringHashList(obj->hashlist[1]);
#endif
			break;
		
		case DICT_APPLICATION:
			free( obj->data.application.application_name );
			break;
			
		case DICT_TYPE:
			free( obj->data.type.type_name );
#if USE_HASHLIST
         switch (obj->data.type.type_base)
         {
            case AVP_TYPE_OCTETSTRING:
               break;
            case AVP_TYPE_INTEGER32:
               deleteInt32HashList(obj->hashlist[0]);
               break;
            case AVP_TYPE_INTEGER64:
               deleteInt64HashList(obj->hashlist[0]);
               break;
            case AVP_TYPE_UNSIGNED32:
               deleteUInt32HashList(obj->hashlist[0]);
               break;
            case AVP_TYPE_UNSIGNED64:
               deleteUInt64HashList(obj->hashlist[0]);
               break;
            case AVP_TYPE_FLOAT32:
               deleteFloat32HashList(obj->hashlist[0]);
               break;
            case AVP_TYPE_FLOAT64:
               deleteFloat64HashList(obj


                     ->hashlist[0]);
               break;
            default:
               break;
         }
         deleteStringHashList(obj->hashlist[1]);
#endif
			break;
			
		case DICT_ENUMVAL:
			free( obj->data.enumval.enum_name );
			break;

		case DICT_AVP:
			free( obj->data.avp.avp_name );
			break;
			
		case DICT_COMMAND:
			free( obj->data.cmd.cmd_name );
			break;
		
		default:
			/* nothing to do */
			;
	}
}

/* Forward declaration */
static void destroy_object(struct dict_object * obj);

/* Destroy all objects in a list - the lock must be held */
static void destroy_list(struct fd_list * head) 
{
	/* TRACE_ENTRY("%p", head); */
	
	/* loop in the list */
	while (!FD_IS_LIST_EMPTY(head))
	{
		/* When destroying the object, it is unlinked from the list */
		destroy_object(_O(head->next->o));
	}
}
	
/* Free an object and its sublists */
static void destroy_object(struct dict_object * obj)
{
	int i;
	
	/* TRACE_ENTRY("%p", obj); */
	
	/* Update global count */
	if (obj->dico) 
		obj->dico->dict_count[obj->type]--;
	
	/* Mark the object as invalid */
	obj->objeyec = 0xdead;
	
	/* First, destroy the data associated to the object */
	destroy_object_data(obj);
	
	for (i=0; i<NB_LISTS_PER_OBJ; i++) {
		if (_OBINFO(obj).haslist[i])
			/* unlink the element from the list */
			fd_list_unlink( &obj->list[i] );
		else
			/* This is either a sentinel or unused (=emtpy) list, let's destroy it */
			destroy_list( &obj->list[i] );
	}
	
	/* Unlink all elements from the dispatch list; they will be freed when callback is unregistered */
	CHECK_POSIX_DO( pthread_rwlock_wrlock(&fd_disp_lock), /* continue */ );
	while (!FD_IS_LIST_EMPTY(&obj->disp_cbs)) {
		fd_list_unlink( obj->disp_cbs.next );
	}
	CHECK_POSIX_DO( pthread_rwlock_unlock(&fd_disp_lock), /* continue */ );
	
	/* Last, destroy the object */
	free(obj);
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/*                                                                                                     */
/*                                  Compare functions                                                  */
/*                                                                                                     */
/*******************************************************************************************************/
/*******************************************************************************************************/

/* Compare two values */
#define ORDER_scalar( i1, i2 ) \
	((i1 < i2 ) ? -1 : ( i1 > i2 ? 1 : 0 )) 


/* Compare two vendor objects by their id (checks already performed) */
static int order_vendor_by_id ( struct dict_object *o1, struct dict_object *o2 )
{
	TRACE_ENTRY("%p %p", o1, o2);
	
	return ORDER_scalar( o1->data.vendor.vendor_id, o2->data.vendor.vendor_id );
}

/* Compare two application objects by their id (checks already performed) */
static int order_appli_by_id  ( struct dict_object *o1, struct dict_object *o2 )
{
	TRACE_ENTRY("%p %p", o1, o2);
	
	return ORDER_scalar( o1->data.application.application_id, o2->data.application.application_id );
}

/* Compare two type objects by their name (checks already performed) */
static int order_type_by_name ( struct dict_object *o1, struct dict_object *o2 )
{
	TRACE_ENTRY("%p %p", o1, o2);
	
	return fd_os_cmp( o1->data.type.type_name, o1->datastr_len, o2->data.type.type_name, o2->datastr_len );
}

/* Compare two type_enum objects by their names (checks already performed) */
static int order_enum_by_name ( struct dict_object *o1, struct dict_object *o2 )
{
	TRACE_ENTRY("%p %p", o1, o2);
	
	return fd_os_cmp( o1->data.enumval.enum_name, o1->datastr_len, o2->data.enumval.enum_name, o2->datastr_len );
}

/* Compare two type_enum objects by their values (checks already performed) */
static int order_enum_by_val  ( struct dict_object *o1, struct dict_object *o2 )
{
	TRACE_ENTRY("%p %p", o1, o2);
	
	/* The comparison function depends on the type of data */
	switch ( o1->parent->data.type.type_base ) {
		case AVP_TYPE_OCTETSTRING:
			return fd_os_cmp( o1->data.enumval.enum_value.os.data, o1->data.enumval.enum_value.os.len, 
					  o2->data.enumval.enum_value.os.data, o2->data.enumval.enum_value.os.len);
		
		case AVP_TYPE_INTEGER32:
			return ORDER_scalar( o1->data.enumval.enum_value.i32, o2->data.enumval.enum_value.i32 );

		case AVP_TYPE_INTEGER64:
			return ORDER_scalar( o1->data.enumval.enum_value.i64, o2->data.enumval.enum_value.i64 );

		case AVP_TYPE_UNSIGNED32:
			return ORDER_scalar( o1->data.enumval.enum_value.u32, o2->data.enumval.enum_value.u32 );

		case AVP_TYPE_UNSIGNED64:
			return ORDER_scalar( o1->data.enumval.enum_value.u64, o2->data.enumval.enum_value.u64 );

		case AVP_TYPE_FLOAT32:
			return ORDER_scalar( o1->data.enumval.enum_value.f32, o2->data.enumval.enum_value.f32 );

		case AVP_TYPE_FLOAT64:
			return ORDER_scalar( o1->data.enumval.enum_value.f64, o2->data.enumval.enum_value.f64 );

		case AVP_TYPE_GROUPED:
		default:
			ASSERT(0);
	}
	return 0;
}

/* Compare two avp objects by their codes (checks already performed) */
static int order_avp_by_code  ( struct dict_object *o1, struct dict_object *o2 )
{
	TRACE_ENTRY("%p %p", o1, o2);
	
	return ORDER_scalar( o1->data.avp.avp_code, o2->data.avp.avp_code );
}

/* Compare two avp objects by their names (checks already performed) */
static int order_avp_by_name  ( struct dict_object *o1, struct dict_object *o2 )
{
	TRACE_ENTRY("%p %p", o1, o2);
	
	return fd_os_cmp( o1->data.avp.avp_name, o1->datastr_len, o2->data.avp.avp_name, o2->datastr_len );
}

/* Compare two command objects by their names (checks already performed) */
static int order_cmd_by_name  ( struct dict_object *o1, struct dict_object *o2 )
{
	TRACE_ENTRY("%p %p", o1, o2);
	
	return fd_os_cmp( o1->data.cmd.cmd_name, o1->datastr_len, o2->data.cmd.cmd_name, o2->datastr_len );
}

/* Compare two command objects by their codes and flags (request or answer) (checks already performed) */
static int order_cmd_by_codefl( struct dict_object *o1, struct dict_object *o2 )
{
	uint8_t fl1, fl2;
	int cmp = 0;
	
	TRACE_ENTRY("%p %p", o1, o2);
	
	cmp = ORDER_scalar( o1->data.cmd.cmd_code, o2->data.cmd.cmd_code );
	if (cmp) 
		return cmp;
	
	/* Same command code, we must compare the value of the 'R' flag */
	fl1 = o1->data.cmd.cmd_flag_val & CMD_FLAG_REQUEST;
	fl2 = o2->data.cmd.cmd_flag_val & CMD_FLAG_REQUEST;
	
	/* We want requests first, so we reverse the operators here */
	return ORDER_scalar(fl2, fl1);
		
}

/* Compare two rule object by the AVP vendor & code that they refer (checks already performed) */
static int order_rule_by_avpvc ( struct dict_object *o1, struct dict_object *o2 )
{
	TRACE_ENTRY("%p %p", o1, o2);
	
	return ORDER_scalar(o1->data.rule.rule_avp->data.avp.avp_vendor, o2->data.rule.rule_avp->data.avp.avp_vendor) 
		?: ORDER_scalar(o1->data.rule.rule_avp->data.avp.avp_code, o2->data.rule.rule_avp->data.avp.avp_code) ;
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/*                                                                                                     */
/*                                  Search  functions                                                  */
/*                                                                                                     */
/*******************************************************************************************************/
/*******************************************************************************************************/

/* Functions used to search for objects in the lists, according to some criteria */

/* On a general note, if result is not NULL, ENOENT is not returned but *result is NULL. */

/* The following macros assume that "what", "ret", "result" (variables), and "end" (label) exist
in the local context where they are called. They are meant to be called only from the functions that follow. */

/* For searchs of type "xxx_OF_xxx": children's parent or default parent */
#define SEARCH_childs_parent( type_of_child, default_parent ) {			\
	struct dict_object *__child = (struct dict_object *) what;		\
	CHECK_PARAMS_DO( verify_object(__child) && 				\
		(__child->type == (type_of_child)), 				\
		   {  ret = EINVAL; goto end;  }  );				\
	ret = 0;								\
	if (result)								\
		*result = (__child->parent ? __child->parent :(default_parent));\
}

/* For search of strings in lists. isindex= 1 if the string is the ordering key of the list */
/* it is expected that object->datastr_len is the length of the datafield parameter */
#define SEARCH_os0_l( str, len, sentinel, datafield, isindex ) {		\
	char *  __str = (char *) (str);						\
	size_t __strlen = (size_t)(len);					\
	int __cmp;								\
	struct fd_list * __li;							\
	ret = 0;								\
	for  (__li = (sentinel)->next; __li != (sentinel); __li = __li->next) {	\
		__cmp = fd_os_cmp(__str, __strlen,				\
			_O(__li->o)->data. datafield, _O(__li->o)->datastr_len);\
		if (__cmp == 0) {						\
			if (result)						\
				*result = _O(__li->o);				\
			goto end;						\
		}								\
		if ((isindex) && (__cmp < 0))					\
			break;							\
	}									\
	if (result)								\
		*result = NULL;							\
	else									\
		ret = ENOENT;							\
}

/* When len is not provided */
#define SEARCH_os0( str, sentinel, datafield, isindex ) {			\
	char *  _str = (char *) (str);						\
	size_t  _strlen = strlen(_str);						\
	SEARCH_os0_l( _str, _strlen, sentinel, datafield, isindex );		\
}


/* For search of octetstrings in lists. */
#define SEARCH_os(  str, strlen, sentinel, osdatafield, isindex ) {		\
	uint8_t * __str = (uint8_t *) (str);					\
	size_t __strlen = (size_t)(strlen);					\
	int __cmp;								\
	struct fd_list * __li;							\
	ret = 0;								\
	for  (__li = (sentinel)->next; __li != (sentinel); __li = __li->next) {	\
		__cmp = fd_os_cmp(__str, __strlen,				\
			_O(__li->o)->data. osdatafield .data,			\
			_O(__li->o)->data. osdatafield .len);			\
		if (__cmp == 0) {						\
			if (result)						\
				*result = _O(__li->o);				\
			goto end;						\
		}								\
		if ((isindex) && (__cmp < 0))					\
			break;							\
	}									\
	if (result)								\
		*result = NULL;							\
	else									\
		ret = ENOENT;							\
}

/* For search of AVP name in rule lists -- the list is not ordered by AVP names! */
#define SEARCH_ruleavpname( str, strlen, sentinel ) {				\
	char * __str = (char *) (str);						\
	size_t __strlen = (size_t) (strlen);					\
	int __cmp;								\
	struct fd_list * __li;							\
	ret = 0;								\
	for  (__li = (sentinel)->next; __li != (sentinel); __li = __li->next) {	\
		__cmp = fd_os_cmp(__str, __strlen, 				\
		  	_O(__li->o)->data.rule.rule_avp->data.avp.avp_name,	\
			_O(__li->o)->data.rule.rule_avp->datastr_len);		\
		if (__cmp == 0) {						\
			if (result)						\
				*result = _O(__li->o);				\
			goto end;						\
		}								\
	}									\
	if (result)								\
		*result = NULL;							\
	else									\
		ret = ENOENT;							\
}

/* For search of scalars in lists. isindex= 1 if the value is the ordering key of the list */
#define SEARCH_scalar( value, sentinel, datafield, isindex, defaultobj ) {	\
	int __cmp;								\
	struct fd_list * __li;							\
	ret = 0;								\
	if (  ((defaultobj) != NULL) 						\
		   && (_O(defaultobj)->data. datafield  == value)) {		\
		if (result)							\
			*result = _O(defaultobj);				\
		goto end;							\
	}									\
	for  (__li = (sentinel)->next; __li != (sentinel); __li = __li->next) {	\
		__cmp= ORDER_scalar(value, _O(__li->o)->data. datafield );	\
		if (__cmp == 0) {						\
			if (result)						\
				*result = _O(__li->o);				\
			goto end;						\
		}								\
		if ((isindex) && (__cmp < 0))					\
			break;							\
	}									\
	if (result)								\
		*result = NULL;							\
	else									\
		ret = ENOENT;							\
}

/* For search of commands in lists by code and flag. R_flag_val = 0 or CMD_FLAG_REQUEST */
#define SEARCH_codefl( value, R_flag_val, sentinel) {					\
	int __cmp;								\
	struct fd_list * __li;							\
	ret = 0;								\
	for  (__li = (sentinel)->next; __li != (sentinel); __li = __li->next) {	\
		__cmp = ORDER_scalar(value, 					\
				_O(__li->o)->data.cmd.cmd_code );		\
		if (__cmp == 0) {						\
			uint8_t __mask, __val;					\
			__mask = _O(__li->o)->data.cmd.cmd_flag_mask;		\
			__val  = _O(__li->o)->data.cmd.cmd_flag_val;		\
			if ( ! (__mask & CMD_FLAG_REQUEST) )			\
				continue;					\
			if ( ( __val & CMD_FLAG_REQUEST ) != R_flag_val )	\
				continue;					\
			if (result)						\
				*result = _O(__li->o);				\
			goto end;						\
		}								\
		if (__cmp < 0)							\
			break;							\
	}									\
	if (result)								\
		*result = NULL;							\
	else									\
		ret = ENOENT;							\
}

/* For searchs of type "xxx_OF_xxx": if the search object is sentinel list for the "what" object */
#define SEARCH_sentinel( type_of_what, what_list_nr, sentinel_list_nr ) {			\
	struct dict_object *__what = (struct dict_object *) what;				\
	CHECK_PARAMS_DO( verify_object(__what) && 						\
		(__what->type == (type_of_what)), 						\
		   {  ret = EINVAL; goto end;  }  );						\
	ret = 0;										\
	if (result) {										\
		/* this is similar to the "container_of" */					\
		*result = (struct dict_object *)((char *)(__what->list[what_list_nr].head) - 	\
		   		(size_t)&(((struct dict_object *)0)->list[sentinel_list_nr]));	\
	}											\
}


static int search_vendor ( struct dictionary * dict, int criteria, const void * what, struct dict_object **result )
{
	int ret = 0;
	vendor_id_t id;
	
	TRACE_ENTRY("%p %d %p %p", dict, criteria, what, result);
	
	switch (criteria) {
		case VENDOR_BY_ID:
			id = *(vendor_id_t *) what;
			SEARCH_scalar( id, &dict->dict_vendors.list[0], vendor.vendor_id, 1, &dict->dict_vendors );
			break;
				
		case VENDOR_BY_NAME:
			/* "what" is a vendor name */
			SEARCH_os0( what, &dict->dict_vendors.list[0], vendor.vendor_name, 0);
			break;
			
		case VENDOR_OF_APPLICATION:
			/* "what" should be an application object */
			SEARCH_childs_parent( DICT_APPLICATION, &dict->dict_vendors );
			break;
		
		case VENDOR_OF_AVP:
			/* "what" should be an avp object */
			SEARCH_sentinel( DICT_AVP, 0, 1 );
			break;
		
		default:
			/* Invalid criteria */
			CHECK_PARAMS( criteria = 0 );
	}
end:
	return ret;
}

static int search_application ( struct dictionary * dict, int criteria, const void * what, struct dict_object **result )
{
	int ret = 0;
	application_id_t id;
	
	TRACE_ENTRY("%p %d %p %p", dict, criteria, what, result);
	
	switch (criteria) {
		case APPLICATION_BY_ID:
			id = *(application_id_t *) what;

			SEARCH_scalar( id, &dict->dict_applications.list[0],  application.application_id, 1, &dict->dict_applications );
			break;
				
		case APPLICATION_BY_NAME:
			/* "what" is an application name */
			SEARCH_os0( what, &dict->dict_applications.list[0], application.application_name, 0);
			break;
			
		case APPLICATION_OF_TYPE:
			/* "what" should be a type object */
			SEARCH_childs_parent( DICT_TYPE, &dict->dict_applications );
			break;
		
		case APPLICATION_OF_COMMAND:
			/* "what" should be a command object */
			SEARCH_childs_parent( DICT_COMMAND, &dict->dict_applications );
			break;
		
		default:
			/* Invalid criteria */
			CHECK_PARAMS( criteria = 0 );
	}
end:
	return ret;
}

static int search_type ( struct dictionary * dict, int criteria, const void * what, struct dict_object **result )
{
	int ret = 0;
	
	TRACE_ENTRY("%p %d %p %p", dict, criteria, what, result);
	
	switch (criteria) {
		case TYPE_BY_NAME:
			/* "what" is a type name */
			SEARCH_os0( what, &dict->dict_types, type.type_name, 1);
			break;
			
		case TYPE_OF_ENUMVAL:
			/* "what" should be a type_enum object */
			SEARCH_childs_parent( DICT_ENUMVAL, NULL );
			break;
		
		case TYPE_OF_AVP:
			/* "what" should be an avp object */
			SEARCH_childs_parent( DICT_AVP, NULL );
			break;
		
				
		default:
			/* Invalid criteria */
			CHECK_PARAMS( criteria = 0 );
	}
end:
	return ret;
}

static int search_enumval ( struct dictionary * dict, int criteria, const void * what, struct dict_object **result )
{
	int ret = 0;
	
	TRACE_ENTRY("%p %d %p %p", dict, criteria, what, result);
	
	switch (criteria) {
		case ENUMVAL_BY_STRUCT:
			{
				struct dict_object * parent = NULL;
				struct dict_enumval_request * _what = (struct dict_enumval_request *) what;
				
				CHECK_PARAMS(  _what  &&  ( _what->type_obj || _what->type_name )  );
				
				if (_what->type_obj != NULL) {
					parent = _what->type_obj;
					CHECK_PARAMS(  verify_object(parent)  &&  (parent->type == DICT_TYPE)  );
				} else {
					/* We received only the type name, we must find it first */
					CHECK_FCT_DO( search_type( dict, TYPE_BY_NAME, _what->type_name, &parent ),
							CHECK_PARAMS( 0 ) );
				}
				
				/* From here the "parent" object is valid */
				
				if ( _what->search.enum_name != NULL ) {
#if USE_HASHLIST
               ret = findStringHashList(_what->search.enum_name, parent->hashlist[1], (void**)result);
#else
					/* We are looking for this string */
					SEARCH_os0(  _what->search.enum_name, &parent->list[1], enumval.enum_name, 1 );
#endif
				} else {
					/* We are looking for the value in enum_value */
					switch (parent->data.type.type_base) {
						case AVP_TYPE_OCTETSTRING:
							SEARCH_os(	 _what->search.enum_value.os.data, 
									 _what->search.enum_value.os.len, 
									 &parent->list[2], 
									 enumval.enum_value.os , 
									 1 );
							break;

						case AVP_TYPE_INTEGER32:
#if USE_HASHLIST
		               ret = findInt32HashList(_what->search.enum_value.i32, parent->hashlist[0], (void**)result);
#else
							SEARCH_scalar(	_what->search.enum_value.i32,
									&parent->list[2],
									enumval.enum_value.i32,
									1,
									(struct dict_object *)NULL);
#endif
							break;
							
						case AVP_TYPE_INTEGER64:
#if USE_HASHLIST
                     ret = findInt64HashList(_what->search.enum_value.i64, parent->hashlist[0], (void**)result);
#else
							SEARCH_scalar(	_what->search.enum_value.i64,
									&parent->list[2],
									enumval.enum_value.i64,
									1,
									(struct dict_object *)NULL);
#endif
							break;
							
						case AVP_TYPE_UNSIGNED32:
#if USE_HASHLIST
                     ret = findUInt32HashList(_what->search.enum_value.u32, parent->hashlist[0], (void**)result);
#else
							SEARCH_scalar(	_what->search.enum_value.u32,
									&parent->list[2],
									enumval.enum_value.u32,
									1,
									(struct dict_object *)NULL);
#endif
							break;
							
						case AVP_TYPE_UNSIGNED64:
#if USE_HASHLIST
                     ret = findUInt64HashList(_what->search.enum_value.u64, parent->hashlist[0], (void**)result);
#else
							SEARCH_scalar(	_what->search.enum_value.u64,
									&parent->list[2],
									enumval.enum_value.u64,
									1,
									(struct dict_object *)NULL);
#endif
							break;
							
						case AVP_TYPE_FLOAT32:
#if USE_HASHLIST
                     ret = findFloat32HashList(_what->search.enum_value.f32, parent->hashlist[0], (void**)result);
#else
							SEARCH_scalar(	_what->search.enum_value.f32,
									&parent->list[2],
									enumval.enum_value.f32,
									1,
									(struct dict_object *)NULL);
#endif
							break;
							
						case AVP_TYPE_FLOAT64:
#if USE_HASHLIST
                     ret = findFloat64HashList(_what->search.enum_value.f64, parent->hashlist[0], (void**)result);
#else
							SEARCH_scalar(	_what->search.enum_value.f64,
									&parent->list[2],
									enumval.enum_value.f64,
									1,
									(struct dict_object *)NULL);
#endif
							break;
							
						default:
							/* Invalid parent type basetype */
							CHECK_PARAMS( parent = NULL );
					}
				}
				
			}
			break;
		
				
		default:
			/* Invalid criteria */
			CHECK_PARAMS( criteria = 0 );
	}
end:
	return ret;
}

static int search_avp ( struct dictionary * dict, int criteria, const void * what, struct dict_object **result )
{
	int ret = 0;
	
	TRACE_ENTRY("%p %d %p %p", dict, criteria, what, result);
	
	switch (criteria) {
		case AVP_BY_CODE:
			{
				avp_code_t code;
				code = *(avp_code_t *) what;

#if USE_HASHLIST
				ret = findUInt32HashList(code, dict->dict_vendors.hashlist[0], (void**)result);
#else
				SEARCH_scalar( code, &dict->dict_vendors.list[1],  avp.avp_code, 1, (struct dict_object *)NULL );
#endif
			}
			break;
				
		case AVP_BY_NAME:
			/* "what" is the AVP name, vendor 0 */
#if USE_HASHLIST
		   ret = findStringHashList((const char *)what, dict->dict_vendors.hashlist[1], (void**)result);
#else
			SEARCH_os0( what, &dict->dict_vendors.list[2], avp.avp_name, 1);
#endif
			break;
			
		case AVP_BY_CODE_AND_VENDOR:
		case AVP_BY_NAME_AND_VENDOR:
			{
				struct dict_avp_request * _what = (struct dict_avp_request *) what;
				struct dict_object * vendor = NULL;
				
				CHECK_PARAMS( (criteria != AVP_BY_NAME_AND_VENDOR) || _what->avp_name  );
				
				/* Now look for the vendor first */
				CHECK_FCT( search_vendor( dict, VENDOR_BY_ID, &_what->avp_vendor, &vendor ) );
				if (vendor == NULL) {
					if (result)
						*result = NULL;
					else
						ret = ENOENT;
					goto end;
				}
				
				/* We now have our vendor = head of the appropriate avp list */
				if (criteria == AVP_BY_NAME_AND_VENDOR) {
#if USE_HASHLIST
				   ret = findStringHashList(_what->avp_name, vendor->hashlist[1], (void**)result);
#else
					SEARCH_os0( _what->avp_name, &vendor->list[2], avp.avp_name, 1);
#endif
				} else {
					/* AVP_BY_CODE_AND_VENDOR */
#if USE_HASHLIST
				   ret = findUInt32HashList(_what->avp_code, vendor->hashlist[0], (void**)result);
#else
					SEARCH_scalar( _what->avp_code, &vendor->list[1], avp.avp_code, 1, (struct dict_object *)NULL );
#endif
				}
			}
			break;
		
		case AVP_BY_STRUCT:
			{
				struct dict_avp_request_ex * _what = (struct dict_avp_request_ex *) what;
				struct dict_object * vendor = NULL;
				
				CHECK_PARAMS( _what->avp_vendor.vendor || _what->avp_vendor.vendor_id || _what->avp_vendor.vendor_name );
				CHECK_PARAMS( _what->avp_data.avp_code || _what->avp_data.avp_name );
				
				/* Now look for the vendor first */
				if (_what->avp_vendor.vendor) {
					CHECK_PARAMS( ! _what->avp_vendor.vendor_id && ! _what->avp_vendor.vendor_name );
					vendor = _what->avp_vendor.vendor;
				} else if (_what->avp_vendor.vendor_id) {
					CHECK_PARAMS( ! _what->avp_vendor.vendor_name );
					CHECK_FCT( search_vendor( dict, VENDOR_BY_ID, &_what->avp_vendor.vendor_id, &vendor ) );
				} else {
					CHECK_FCT( search_vendor( dict, VENDOR_BY_NAME, _what->avp_vendor.vendor_name, &vendor ) );
				}
				
				if (vendor == NULL) {
					if (result)
						*result = NULL;
					else
						ret = ENOENT;
					goto end;
				}
				
				/* We now have our vendor = head of the appropriate avp list */
				if (_what->avp_data.avp_code) {
					CHECK_PARAMS( ! _what->avp_data.avp_name );
#if USE_HASHLIST
					ret = findUInt32HashList(_what->avp_data.avp_code, vendor->hashlist[0], (void**)result);
#else
					SEARCH_scalar( _what->avp_data.avp_code, &vendor->list[1], avp.avp_code, 1, (struct dict_object *)NULL );
#endif
				} else {
#if USE_HASHLIST
				   ret = findStringHashList(_what->avp_data.avp_name, vendor->hashlist[1], (void**)result);
#else
					SEARCH_os0( _what->avp_data.avp_name, &vendor->list[2], avp.avp_name, 1);
#endif
				}
			}
			break;
		
		case AVP_BY_NAME_ALL_VENDORS:
			{
				struct fd_list * li;
				size_t wl = strlen((char *)what);
				
				/* First, search for vendor 0 */
				SEARCH_os0_l( what, wl, &dict->dict_vendors.list[2], avp.avp_name, 1);
				
				/* If not found, loop for all vendors, until found */
				for (li = dict->dict_vendors.list[0].next; li != &dict->dict_vendors.list[0]; li = li->next) {
#if USE_HASHLIST
               ret = findStringHashList(what, _O(li->o)->hashlist[1], (void**)result);
               if (ret == 0 && *result)
                  goto end;
#else
					SEARCH_os0_l( what, wl, &_O(li->o)->list[2], avp.avp_name, 1);
#endif
				}
			}
			break;
		
		default:
			/* Invalid criteria */
			CHECK_PARAMS( criteria = 0 );
	}
end:
	return ret;
}

static int search_cmd ( struct dictionary * dict, int criteria, const void * what, struct dict_object **result )
{
	int ret = 0;
	
	TRACE_ENTRY("%p %d %p %p", dict, criteria, what, result);
	
	switch (criteria) {
		case CMD_BY_NAME:
			/* "what" is a command name */
			SEARCH_os0( what, &dict->dict_cmd_name, cmd.cmd_name, 1);
			break;
			
		case CMD_BY_CODE_R:
		case CMD_BY_CODE_A:
			{
				command_code_t code;
				uint8_t searchfl = 0;
				
				/* The command code that we are searching */
				code = *(command_code_t *) what;
				
				/* The flag (request or answer) of the command we are searching */
				if (criteria == CMD_BY_CODE_R) {
					searchfl = CMD_FLAG_REQUEST;
				}
				
				/* perform the search */
				SEARCH_codefl( code, searchfl, &dict->dict_cmd_code );
			}
			break;
				
		case CMD_ANSWER:
			{
				/* "what" is a command object of type "request" */
				struct dict_object * req = (struct dict_object *) what;
				struct dict_object * ans = NULL;
				
				CHECK_PARAMS( verify_object(req) 
						&& (req->type == DICT_COMMAND)
						&& (req->data.cmd.cmd_flag_mask & CMD_FLAG_REQUEST)
						&& (req->data.cmd.cmd_flag_val  & CMD_FLAG_REQUEST) );
				
				/* The answer is supposed to be the next element in the list, if it exists */
				ans = req->list[1].next->o;
				if ( ans == NULL ) {
					TRACE_DEBUG( FULL, "the request was the last element in the list" );
					ret = ENOENT;
					goto end;
				}
				
				/* Now check that the ans element is really the correct one */
				if (  (ans->data.cmd.cmd_code != req->data.cmd.cmd_code)
				   || (!(ans->data.cmd.cmd_flag_mask & CMD_FLAG_REQUEST))
				   || (  ans->data.cmd.cmd_flag_val  & CMD_FLAG_REQUEST ) ) {
					TRACE_DEBUG( FULL, "the answer does not follow the request in the list" );
					ret = ENOENT;
					goto end;
				}
				
				if (result)
					*result = ans;
				ret = 0;
			}						
			break;
			
		default:
			/* Invalid criteria */
			CHECK_PARAMS( criteria = 0 );
	}
end:
	return ret;
}

static int search_rule ( struct dictionary * dict, int criteria, const void * what, struct dict_object **result )
{
	int ret = 0;
	
	TRACE_ENTRY("%p %d %p %p", dict, criteria, what, result);
	
	switch (criteria) {
		case RULE_BY_AVP_AND_PARENT:
			{
				struct dict_object * parent = NULL;
				struct dict_object * avp = NULL;
				struct dict_rule_request * _what = (struct dict_rule_request *) what;
				
				CHECK_PARAMS( _what 
						&& (parent = _what->rule_parent)
						&& (avp    = _what->rule_avp   ) );
				
				CHECK_PARAMS( verify_object(parent) 
						&& ((parent->type == DICT_COMMAND) 
						 || ((parent->type == DICT_AVP) && (parent->data.avp.avp_basetype == AVP_TYPE_GROUPED))) );
				
				CHECK_PARAMS( verify_object(avp) && (avp->type == DICT_AVP) );
				
				/* Perform the search */
				SEARCH_ruleavpname( avp->data.avp.avp_name, avp->datastr_len, &parent->list[2]);
				
			}
			break;
			
		default:
			/* Invalid criteria */
			CHECK_PARAMS( criteria = 0 );
	}
end:
	return ret;
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/*                                                                                                     */
/*                                  Dump / debug functions                                             */
/*                                                                                                     */
/*******************************************************************************************************/
/*******************************************************************************************************/
/* The following functions are used to debug the module, and allow to print out the content of the dictionary */
static DECLARE_FD_DUMP_PROTOTYPE(dump_vendor_data, void * data )
{
	struct dict_vendor_data * vendor = (struct dict_vendor_data *)data;
	
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "data: %-6u \"%s\"", vendor->vendor_id, vendor->vendor_name);
}
static DECLARE_FD_DUMP_PROTOTYPE(dump_application_data, void * data )
{
	struct dict_application_data * appli = (struct dict_application_data *) data;
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "data: %-6u \"%s\"", appli->application_id, appli->application_name);
}
static DECLARE_FD_DUMP_PROTOTYPE(dump_type_data, void * data )
{
	struct dict_type_data * type = ( struct dict_type_data * ) data;
	
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "data: %-12s \"%s\"", 
			type_base_name[type->type_base], 
			type->type_name);
}
static DECLARE_FD_DUMP_PROTOTYPE(dump_enumval_data, struct dict_enumval_data * enumval, enum dict_avp_basetype type )
{
	const int LEN_MAX = 20;
	CHECK_MALLOC_DO(fd_dump_extend( FD_DUMP_STD_PARAMS, "data: (%-12s) \"%s\" -> ", type_base_name[type], enumval->enum_name), return NULL);
	switch (type) {
		case AVP_TYPE_OCTETSTRING:
			{
				int i, n=LEN_MAX;
				if (enumval->enum_value.os.len < LEN_MAX)
					n = enumval->enum_value.os.len;
				for (i=0; i < n; i++)
					CHECK_MALLOC_DO(fd_dump_extend( FD_DUMP_STD_PARAMS, "0x%2hhX/'%c' ", enumval->enum_value.os.data[i], ASCII(enumval->enum_value.os.data[i])), return NULL);
				if (n == LEN_MAX)
					CHECK_MALLOC_DO(fd_dump_extend( FD_DUMP_STD_PARAMS, "..."), return NULL);
			}
			break;
		
		case AVP_TYPE_INTEGER32:
			CHECK_MALLOC_DO(fd_dump_extend( FD_DUMP_STD_PARAMS, "%i", enumval->enum_value.i32), return NULL);
			break;

		case AVP_TYPE_INTEGER64:
			CHECK_MALLOC_DO(fd_dump_extend( FD_DUMP_STD_PARAMS, "%"PRId64, enumval->enum_value.i64), return NULL);
			break;

		case AVP_TYPE_UNSIGNED32:
			CHECK_MALLOC_DO(fd_dump_extend( FD_DUMP_STD_PARAMS, "%u", enumval->enum_value.u32), return NULL);
			break;

		case AVP_TYPE_UNSIGNED64:
			CHECK_MALLOC_DO(fd_dump_extend( FD_DUMP_STD_PARAMS, "%"PRIu64, enumval->enum_value.u64), return NULL);
			break;

		case AVP_TYPE_FLOAT32:
			CHECK_MALLOC_DO(fd_dump_extend( FD_DUMP_STD_PARAMS, "%f", enumval->enum_value.f32), return NULL);
			break;

		case AVP_TYPE_FLOAT64:
			CHECK_MALLOC_DO(fd_dump_extend( FD_DUMP_STD_PARAMS, "%g", enumval->enum_value.f64), return NULL);
			break;
		
		default:
			CHECK_MALLOC_DO(fd_dump_extend( FD_DUMP_STD_PARAMS, "??? (ERROR unknown type %d)", type), return NULL);
	}
	return *buf;
}
static DECLARE_FD_DUMP_PROTOTYPE(dump_avp_data, void * data )
{
	struct dict_avp_data * avp = (struct dict_avp_data * ) data;
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "data: v/m:" DUMP_AVPFL_str "/" DUMP_AVPFL_str ", %12s, %-6u \"%s\"", 
			DUMP_AVPFL_val(avp->avp_flag_val), 
			DUMP_AVPFL_val(avp->avp_flag_mask), 
			type_base_name[avp->avp_basetype], 
			avp->avp_code, 
			avp->avp_name );
}
static DECLARE_FD_DUMP_PROTOTYPE(dump_command_data, void * data )
{
	struct dict_cmd_data * cmd = (struct dict_cmd_data *) data;
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "data: v/m:" DUMP_CMDFL_str "/" DUMP_CMDFL_str ", %-6u \"%s\"", 
			DUMP_CMDFL_val(cmd->cmd_flag_val), DUMP_CMDFL_val(cmd->cmd_flag_mask), cmd->cmd_code, cmd->cmd_name);
}
static DECLARE_FD_DUMP_PROTOTYPE(dump_rule_data, void * data )
{
	struct dict_rule_data * rule = (struct dict_rule_data * )data;
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "data: pos:%d ord:%d m/M:%2d/%2d avp:\"%s\"",
			rule->rule_position, 
			rule->rule_order, 
			rule->rule_min, 
			rule->rule_max,
			rule->rule_avp->data.avp.avp_name);
}

static DECLARE_FD_DUMP_PROTOTYPE(dump_object, struct dict_object * obj, int parents, int depth, int indent );

static DECLARE_FD_DUMP_PROTOTYPE(dump_list, struct fd_list * sentinel, int parents, int depth, int indent )
{
	struct fd_list * li = sentinel;
	/* We don't lock here, the caller must have taken the dictionary lock for reading already */
	if (FD_IS_LIST_EMPTY(sentinel)) {
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "\n%*s{empty list}", indent, ""), return NULL);
	} else {
		while (li->next != sentinel)
		{
			li = li->next;
			CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "\n"), return NULL);
			CHECK_MALLOC_DO( dump_object (FD_DUMP_STD_PARAMS, _O(li->o), parents, depth, indent ), return NULL);
		}
	}
	return *buf;
}

static DECLARE_FD_DUMP_PROTOTYPE(dump_object, struct dict_object * obj, int parents, int depth, int indent )
{
	CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "%*s{dictobj}(@%p): ", indent, "", obj), return NULL);
	
	if (!verify_object(obj)) {
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "INVALID/NULL"), return NULL);
		return *buf;
	}
	
	CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "%s p:%p ", 
								_OBINFO(obj).name, 
								obj->parent), return NULL);
	
	if (obj->type == DICT_ENUMVAL) {
		CHECK_MALLOC_DO( dump_enumval_data ( FD_DUMP_STD_PARAMS, &obj->data.enumval, obj->parent->data.type.type_base ), return NULL);
	} else {
		CHECK_MALLOC_DO( _OBINFO(obj).dump_data(FD_DUMP_STD_PARAMS, &obj->data), return NULL);
	}
	
	if (parents) {
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "\n%*sparent:", indent + 1, ""), return NULL);
		CHECK_MALLOC_DO( dump_object (FD_DUMP_STD_PARAMS, obj->parent, parents-1, 0, 0 ), return NULL);
	}
	
	if (depth) {
		int i;
		for (i=0; i<NB_LISTS_PER_OBJ; i++) {
			if ((obj->list[i].o == NULL) && (obj->list[i].next != &obj->list[i])) {
				CHECK_MALLOC_DO( dump_list(FD_DUMP_STD_PARAMS, &obj->list[i], 0, depth - 1, indent + 2), return NULL);
				break; /* we get duplicate information sorted by another criteria otherwise, which is not very useful */
			}
		}
	}
	
	return *buf;
}

DECLARE_FD_DUMP_PROTOTYPE(fd_dict_dump_object, struct dict_object * obj)
{
	FD_DUMP_HANDLE_OFFSET();
	
	CHECK_MALLOC_DO( dump_object(FD_DUMP_STD_PARAMS, obj, 1, 2, 0), return NULL);
	
	return *buf;
}

DECLARE_FD_DUMP_PROTOTYPE(fd_dict_dump, struct dictionary * dict)
{
	int i;
	struct fd_list * li;
	
	FD_DUMP_HANDLE_OFFSET();
		
	CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "{dictionary}(@%p): ", dict), return NULL);
	
	if ((dict == NULL) || (dict->dict_eyec != DICT_EYECATCHER)) {
		return fd_dump_extend(FD_DUMP_STD_PARAMS, "INVALID/NULL");
	}
	
#if ENABLE_LOCK_BYPASS
	if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX_DO(  pthread_rwlock_rdlock( &dict->dict_lock ), /* ignore */  );
	
	CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "\n {dict(%p) : VENDORS / AVP / RULES}\n", dict), goto error);
	CHECK_MALLOC_DO( dump_object (FD_DUMP_STD_PARAMS, &dict->dict_vendors, 0, 3, 3 ), goto error);
	for (li = dict->dict_vendors.list[0].next; li != &dict->dict_vendors.list[0]; li = li->next) {
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "\n"), return NULL);
		CHECK_MALLOC_DO( dump_object (FD_DUMP_STD_PARAMS, li->o, 0, 3, 3 ), goto error);
	}
	
	CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "\n {dict(%p) : APPLICATIONS}\n", dict), goto error);
	CHECK_MALLOC_DO( dump_object (FD_DUMP_STD_PARAMS, &dict->dict_applications, 0, 1, 3 ), goto error);
	for (li = dict->dict_applications.list[0].next; li != &dict->dict_applications.list[0]; li = li->next) {
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "\n"), return NULL);
		CHECK_MALLOC_DO( dump_object (FD_DUMP_STD_PARAMS, li->o, 0, 1, 3 ), goto error);
	}
	
	CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "\n {dict(%p) : TYPES / ENUMVAL}", dict), goto error);
	CHECK_MALLOC_DO( dump_list(FD_DUMP_STD_PARAMS, &dict->dict_types, 0, 2, 3 ), goto error);
	
	CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "\n {dict(%p) : COMMANDS / RULES}", dict), goto error);
	CHECK_MALLOC_DO( dump_list(FD_DUMP_STD_PARAMS, &dict->dict_cmd_code, 0, 0, 3 ), goto error);
	
	CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "\n {dict(%p) : statistics}", dict), goto error);
	for (i=1; i<=DICT_TYPE_MAX; i++)
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "\n   %5d: %s",  dict->dict_count[i], dict_obj_info[i].name), goto error);
	
#if ENABLE_LOCK_BYPASS
   if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX_DO(  pthread_rwlock_unlock( &dict->dict_lock ), /* ignore */  );
	return *buf;
error:	
	/* Free the rwlock */
#if ENABLE_LOCK_BYPASS
   if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX_DO(  pthread_rwlock_unlock( &dict->dict_lock ), /* ignore */  );
	return NULL;
}

/**************************** Dump AVP values ********************************/

/* Default dump functions */
static DECLARE_FD_DUMP_PROTOTYPE(dump_val_os, union avp_value * value)
{
	int i;
	
	CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "<"), return NULL);
	for (i = 0; i < value->os.len; i++) {
		if (i == 1024) { /* Dump only up to 1024 bytes of the buffer */
			CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "[...] (len=%zd)", value->os.len), return NULL);
			break;
		}
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "%s%02hhX", (i==0 ? "" : " "), value->os.data[i]), return NULL);
	}
	CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, ">"), return NULL);
	return *buf;
}

static DECLARE_FD_DUMP_PROTOTYPE(dump_val_i32, union avp_value * value)
{
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "%i (0x%x)", value->i32, value->i32);
}

static DECLARE_FD_DUMP_PROTOTYPE(dump_val_i64, union avp_value * value)
{
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "%" PRId64 " (0x%" PRIx64 ")", value->i64, value->i64);
}

static DECLARE_FD_DUMP_PROTOTYPE(dump_val_u32, union avp_value * value)
{
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "%u (0x%x)", value->u32, value->u32);
}

static DECLARE_FD_DUMP_PROTOTYPE(dump_val_u64, union avp_value * value)
{
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "%" PRIu64 " (0x%" PRIx64 ")", value->u64, value->u64);
}

static DECLARE_FD_DUMP_PROTOTYPE(dump_val_f32, union avp_value * value)
{
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "%f", value->f32);
}

static DECLARE_FD_DUMP_PROTOTYPE(dump_val_f64, union avp_value * value)
{
	return fd_dump_extend( FD_DUMP_STD_PARAMS, "%g", value->f64);
}

/* Get the dump function for basic dict_avp_basetype */
static DECLARE_FD_DUMP_PROTOTYPE((*get_default_dump_val_cb(enum dict_avp_basetype datatype)), union avp_value *)
{
	switch (datatype) {
		case AVP_TYPE_OCTETSTRING:
			return &dump_val_os;
		
		case AVP_TYPE_INTEGER32:
			return &dump_val_i32;

		case AVP_TYPE_INTEGER64:
			return &dump_val_i64;

		case AVP_TYPE_UNSIGNED32:
			return &dump_val_u32;

		case AVP_TYPE_UNSIGNED64:
			return &dump_val_u64;

		case AVP_TYPE_FLOAT32:
			return &dump_val_f32;

		case AVP_TYPE_FLOAT64:
			return &dump_val_f64;
		
		case AVP_TYPE_GROUPED:
			TRACE_DEBUG(FULL, "error: grouped AVP with a value!");
	}
	return NULL;
}

/* indent inside an object (duplicate from messages.c) */
#define INOBJHDR 	"%*s   "
#define INOBJHDRVAL 	indent<0 ? 1 : indent, indent<0 ? "-" : "|"

typedef DECLARE_FD_DUMP_PROTOTYPE((*dump_val_cb_t), union avp_value *);

/* Formatter for the AVP value dump line */
static DECLARE_FD_DUMP_PROTOTYPE(dump_avp_val, union avp_value *avp_value, 
			dump_val_cb_t def_dump_val_cb, 
			dump_val_cb_t dump_val_cb, 
			enum dict_avp_basetype datatype, 
			char * type_name, 
			char * const_name, 
			int indent, 
		        int header)
{
	if (header) {
		/* Header for all AVP values dumps: */
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, INOBJHDR "value ", INOBJHDRVAL), return NULL);
	
		/* If the type is provided, write it */
		if (type_name) {
			CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "t: '%s' ", type_name), return NULL);
		}
	
		/* Always give the base datatype anyway */
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "(%s) ", type_base_name[datatype]), return NULL);

		/* Now, the value */
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "v: "), return NULL);
	}
	if (const_name) {
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "'%s' (", const_name), return NULL);
	}
	if (dump_val_cb) {
		CHECK_MALLOC_DO( (*dump_val_cb)( FD_DUMP_STD_PARAMS, avp_value), fd_dump_extend( FD_DUMP_STD_PARAMS, "(dump failed)"));
	} else {
		CHECK_MALLOC_DO( (*def_dump_val_cb)( FD_DUMP_STD_PARAMS, avp_value), return NULL);
	}
	if (const_name) {
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, ")"), return NULL);
	}
	
	/* Done! */
	return *buf;
}

/* Dump the value of an AVP of known type into the returned str */
DECLARE_FD_DUMP_PROTOTYPE(fd_dict_dump_avp_value, union avp_value *avp_value, struct dict_object * model, int indent, int header)
{
	DECLARE_FD_DUMP_PROTOTYPE((*dump_val_cb), union avp_value *avp_value) = NULL;
	struct dict_object * type = NULL;
	char * type_name = NULL;
	char * const_name = NULL;
	
	FD_DUMP_HANDLE_OFFSET();
	
	/* Handle invalid parameters */
	if (!avp_value) {
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "(avp value not set)"), return NULL);
		return *buf;
	}

	if (!model) {
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "(model not set)"), return NULL);
		return *buf;
	}
	
	if (! (	verify_object(model) && (model->type == DICT_AVP) )) {
		CHECK_MALLOC_DO( fd_dump_extend( FD_DUMP_STD_PARAMS, "(invalid model)"), return NULL);
		return *buf;
	}
	
	/* Get the type definition of this AVP */
	type = model->parent;
	if (type) {
		struct dict_enumval_request  request;
		struct dict_object * enumval = NULL;
		
		type_name = type->data.type.type_name;
		
		/* overwrite the dump function ? */
		if (type->data.type.type_dump)
			dump_val_cb = type->data.type.type_dump;
		
		/* Now check if the AVP value matches a constant */
		memset(&request, 0, sizeof(request));
		request.type_obj = type;
		memcpy(&request.search.enum_value, avp_value, sizeof(union avp_value));
		/* bypass checks */
		if ((search_enumval( type->dico, ENUMVAL_BY_STRUCT, &request, &enumval ) == 0) && (enumval)) {
			/* We found a constant, get its name */
			const_name = enumval->data.enumval.enum_name;
		}
	}
	
	/* And finally, dump the value */
	CHECK_MALLOC_DO( dump_avp_val(FD_DUMP_STD_PARAMS, avp_value, get_default_dump_val_cb(model->data.avp.avp_basetype), dump_val_cb, model->data.avp.avp_basetype, type_name, const_name, indent, header), return NULL );
	return *buf;
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/*                                                                                                     */
/*                                  Exported functions                                                 */
/*                                                                                                     */
/*******************************************************************************************************/
/*******************************************************************************************************/

/* These are the functions exported outside libfreeDiameter. */

/* Get the data associated to an object */
int fd_dict_gettype ( struct dict_object * object, enum dict_object_type * type)
{
	TRACE_ENTRY("%p %p", object, type);
	
	CHECK_PARAMS( type && verify_object(object) );
	
	/* Copy the value and return */
	*type = object->type;
	return 0;
}

int fd_dict_getdict ( struct dict_object * object, struct dictionary ** dict)
{
	TRACE_ENTRY("%p %p", object, dict);
	
	CHECK_PARAMS( dict && verify_object(object) );
	
	/* Copy the value and return */
	*dict = object->dico;
	return 0;
}


/* Get the data associated to an object */
int fd_dict_getval ( struct dict_object * object, void * val)
{
	TRACE_ENTRY("%p %p", object, val);
	
	CHECK_PARAMS( val && verify_object(object) );
	
	/* Copy the value and return */
	memcpy(val, &object->data, _OBINFO(object).datasize);;
	return 0;
}

/* Add a new object in the dictionary */
int fd_dict_new ( struct dictionary * dict, enum dict_object_type type, void * data, struct dict_object * parent, struct dict_object **ref )
{
	int ret = 0;
	int dupos = 0;
	struct dict_object * new = NULL;
	struct dict_object * vendor = NULL;
	struct dict_object * locref = NULL;
	
	TRACE_ENTRY("%p %d(%s) %p %p %p", dict, type, dict_obj_info[CHECK_TYPE(type) ? type : 0].name, data, parent, ref);
	
	/* Check parameters */
	CHECK_PARAMS( dict && (dict->dict_eyec == DICT_EYECATCHER) && CHECK_TYPE(type) && data  );
	
	/* Check the "parent" parameter */
	switch (dict_obj_info[type].parent) {
		case 0:	/* parent is forbidden */
			CHECK_PARAMS_DO( parent == NULL, goto error_param );
		
		case 1:	/* parent is optional */
			if (parent == NULL)
				break;
		
		case 2: /* parent is mandatory */
			CHECK_PARAMS_DO(  verify_object(parent), goto error_param  );
			
			if (type == DICT_RULE ) { /* Special case : grouped AVP or Command parents are allowed */
				CHECK_PARAMS_DO( (parent->type == DICT_COMMAND ) 
						|| ( (parent->type == DICT_AVP) && (parent->data.avp.avp_basetype == AVP_TYPE_GROUPED ) ), goto error_param );
			} else {
				CHECK_PARAMS_DO( parent->type == dict_obj_info[type].parenttype, goto error_param );
			}
	}
	
	/* For AVP object, we must also check that the "vendor" referenced exists */
	if (type == DICT_AVP) {
		CHECK_FCT_DO(  fd_dict_search( dict, DICT_VENDOR, VENDOR_BY_ID, &(((struct dict_avp_data *)data)->avp_vendor), (void*)&vendor, ENOENT ),
			{ TRACE_DEBUG(INFO, "Unable to find vendor '%d' referenced in the AVP data", ((struct dict_avp_data *)data)->avp_vendor); goto error_param; }  );
		
		/* Also check if a parent is provided, that the type are the same */
		if (parent) {
			CHECK_PARAMS_DO(  parent->data.type.type_base == ((struct dict_avp_data *)data)->avp_basetype, goto error_param  );
		}
	}
	
	/* For RULE object, we must also check that the "avp" referenced exists */
	if (type == DICT_RULE) {
		CHECK_PARAMS_DO(  verify_object(((struct dict_rule_data *)data)->rule_avp), goto error_param  );
		CHECK_PARAMS_DO(  ((struct dict_rule_data *)data)->rule_avp->type == DICT_AVP, goto error_param  );
	}
	
	/* For COMMAND object, check that the 'R' flag is fixed */
	if (type == DICT_COMMAND) {
		CHECK_PARAMS_DO( ((struct dict_cmd_data *)data)->cmd_flag_mask & CMD_FLAG_REQUEST, goto error_param   );
	}

	/* For ENUMVAL object, check if the parent type is an OctetString */
	if (type == DICT_ENUMVAL) {
		if (parent->data.type.type_base == AVP_TYPE_OCTETSTRING)
			dupos = 1;
	}
	
	/* We have to check that the new values are not equal to the sentinels */
	if (type == DICT_VENDOR) {
		CHECK_PARAMS_DO( ((struct dict_vendor_data *)data)->vendor_id != 0, goto error_param   );
	}
	if (type == DICT_APPLICATION) {
		CHECK_PARAMS_DO( ((struct dict_application_data *)data)->application_id != 0, goto error_param   );
	}
	
	/* Parameters are valid, create the new object */
	CHECK_MALLOC(  new = malloc(sizeof(struct dict_object))  );
	
	/* Initialize the data of the new object */
	init_object(new, type);
   new->dico = dict;
   new->parent = parent;
	init_object_data(new, data, type, dupos);

	/* We will change the dictionary => acquire the write lock */
#if ENABLE_LOCK_BYPASS
	if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX_DO(  ret = pthread_rwlock_wrlock(&dict->dict_lock),  goto error_free  );
	
	/* Now link the object -- this also checks that no object with same keys already exists */
	switch (type) {
		case DICT_VENDOR:
			/* A vendor object is linked in the g_dict_vendors.list[0], by their id */
			ret = fd_list_insert_ordered ( &dict->dict_vendors.list[0], &new->list[0], (int (*)(void*, void *))order_vendor_by_id, (void **)&locref );
			if (ret)
				goto error_unlock;
			break;
		
		case DICT_APPLICATION:
			/* An application object is linked in the g_dict_applciations.list[0], by their id */
			ret = fd_list_insert_ordered ( &dict->dict_applications.list[0], &new->list[0], (int (*)(void*, void *))order_appli_by_id, (void **)&locref );
			if (ret)
				goto error_unlock;
			break;
		
		case DICT_TYPE:
			/* A type object is linked in g_list_types by its name */
			ret = fd_list_insert_ordered ( &dict->dict_types, &new->list[0], (int (*)(void*, void *))order_type_by_name, (void **)&locref );
			if (ret)
				goto error_unlock;
			break;
		
		case DICT_ENUMVAL:
#if USE_HASHLIST
		   switch (parent->data.type.type_base)
		   {
            case AVP_TYPE_INTEGER32:
               ret = insertInt32HashList(new->data.enumval.enum_value.i32, new, parent->hashlist[0], (void**)&locref);
               break;

            case AVP_TYPE_INTEGER64:
               ret = insertInt64HashList(new->data.enumval.enum_value.i64, new, parent->hashlist[0], (void**)&locref);
               break;

            case AVP_TYPE_UNSIGNED32:
               ret = insertUInt32HashList(new->data.enumval.enum_value.u32, new, parent->hashlist[0], (void**)&locref);
               break;

            case AVP_TYPE_UNSIGNED64:
               ret = insertUInt64HashList(new->data.enumval.enum_value.u64, new, parent->hashlist[0], (void**)&locref);
               break;

            case AVP_TYPE_FLOAT32:
               ret = insertFloat32HashList(new->data.enumval.enum_value.f32, new, parent->hashlist[0], (void**)&locref);
               break;

            case AVP_TYPE_FLOAT64:
               ret = insertFloat64HashList(new->data.enumval.enum_value.f64, new, parent->hashlist[0], (void**)&locref);
               break;

            default:
               /* Invalid parent type basetype */
               CHECK_PARAMS( parent = NULL );
		   }
		   if (ret)
		      goto error_unlock;

         ret = insertStringHashList(new->data.enumval.enum_name, new, parent->hashlist[1], (void **)&locref);
         if (ret) {
            switch (parent->data.type.type_base)
            {
               case AVP_TYPE_INTEGER32:
                  deleteEntryInt32HashList(new->data.enumval.enum_value.i32, parent->hashlist[0]);
                  break;

               case AVP_TYPE_INTEGER64:
                  deleteEntryInt64HashList(new->data.enumval.enum_value.i64, parent->hashlist[0]);
                  break;

               case AVP_TYPE_UNSIGNED32:
                  deleteEntryUInt32HashList(new->data.enumval.enum_value.u32, parent->hashlist[0]);
                  break;

               case AVP_TYPE_UNSIGNED64:
                  deleteEntryUInt64HashList(new->data.enumval.enum_value.u64, parent->hashlist[0]);
                  break;

               case AVP_TYPE_FLOAT32:
                  deleteEntryFloat32HashList(new->data.enumval.enum_value.f32, parent->hashlist[0]);
                  break;

               case AVP_TYPE_FLOAT64:
                  deleteEntryFloat64HashList(new->data.enumval.enum_value.f64, parent->hashlist[0]);
                  break;

               default:
                  /* Invalid parent type basetype */
                  CHECK_PARAMS( parent = NULL );
            }
            goto error_unlock;
         }
#endif
			/* A type_enum object is linked in it's parent 'type' object lists 1 and 2 by its name and values */
			ret = fd_list_insert_ordered ( &parent->list[1], &new->list[0], (int (*)(void*, void *))order_enum_by_name, (void **)&locref );
			if (ret)
				goto error_unlock;
			
			ret = fd_list_insert_ordered ( &parent->list[2], &new->list[1], (int (*)(void*, void *))order_enum_by_val, (void **)&locref );
			if (ret) { 
				fd_list_unlink(&new->list[0]); 
				goto error_unlock; 
			}
			break;
		
		case DICT_AVP:
#if USE_HASHLIST
		   ret = insertUInt32HashList(new->data.avp.avp_code, new, vendor->hashlist[0], (void **)&locref);
		   if (ret)
		      goto error_unlock;

		   ret = insertStringHashList(new->data.avp.avp_name, new, vendor->hashlist[1], (void **)&locref);
		   if (ret) {
		      deleteEntryUInt32HashList(new->data.avp.avp_code, vendor->hashlist[0]);
		      goto error_unlock;
		   }
#endif
			/* An avp object is linked in lists 1 and 2 of its vendor, by code and name */
			ret = fd_list_insert_ordered ( &vendor->list[1], &new->list[0], (int (*)(void*, void *))order_avp_by_code, (void **)&locref );
			if (ret)
				goto error_unlock;
			
			ret = fd_list_insert_ordered ( &vendor->list[2], &new->list[1], (int (*)(void*, void *))order_avp_by_name, (void **)&locref );
			if (ret) {
				fd_list_unlink(&new->list[0]);
				goto error_unlock;
			}
			break;
			
		case DICT_COMMAND:
			/* A command object is linked in g_list_cmd_name and g_list_cmd_code by its name and code */
			ret = fd_list_insert_ordered ( &dict->dict_cmd_code, &new->list[1], (int (*)(void*, void *))order_cmd_by_codefl, (void **)&locref );
			if (ret)
				goto error_unlock;
			
			ret = fd_list_insert_ordered ( &dict->dict_cmd_name, &new->list[0], (int (*)(void*, void *))order_cmd_by_name, (void **)&locref );
			if (ret) {
				fd_list_unlink(&new->list[1]);
				goto error_unlock;
			}
			break;
		
		case DICT_RULE:
			/* A rule object is linked in list[2] of its parent command or AVP by the name of the AVP it refers */
			ret = fd_list_insert_ordered ( &parent->list[2], &new->list[0], (int (*)(void*, void *))order_rule_by_avpvc, (void **)&locref );
			if (ret){
			   /*If the new rule is optional and the existing one is mandatory ==> override the existing rule to optional */
			   if(locref->data.rule.rule_position == RULE_REQUIRED && new->data.rule.rule_position == RULE_OPTIONAL){
			      TRACE_DEBUG(INFO, "Overriding rule to optional for AVP: %s", locref->data.rule.rule_avp->data.avp.avp_name);
			      locref->data.rule.rule_position = RULE_OPTIONAL;
			   }
				goto error_unlock;
			}
			break;
			
		default:
			ASSERT(0);
	}
	
	/* A new object has been created, increment the global counter */
	dict->dict_count[type]++;
	
	/* Unlock the dictionary */
#if ENABLE_LOCK_BYPASS
	if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX_DO(  ret = pthread_rwlock_unlock(&dict->dict_lock),  goto error_free  );
	
	/* Save the pointer to the new object */
	if (ref)
		*ref = new;
	
	return 0;
	
error_param:
	ret = EINVAL;
	goto all_errors;

error_unlock:
#if ENABLE_LOCK_BYPASS
   if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX_DO(  pthread_rwlock_unlock(&dict->dict_lock),  /* continue */  );
	if (ret == EEXIST) {
		/* We have a duplicate key in locref. Check if the pointed object is the same or not */
		switch (type) {
			case DICT_VENDOR:
				TRACE_DEBUG(FULL, "Vendor %s already in dictionary", new->data.vendor.vendor_name);
				/* if we are here, it means the two vendors id are identical */
				if (fd_os_cmp(locref->data.vendor.vendor_name, locref->datastr_len, 
						new->data.vendor.vendor_name, new->datastr_len)) {
					TRACE_DEBUG(INFO, "Conflicting vendor name: %s", new->data.vendor.vendor_name);
					break;
				}
				/* Otherwise (same name), we consider the function succeeded, since the (same) object is in the dictionary */
				ret = 0; 
				break;

			case DICT_APPLICATION:
				TRACE_DEBUG(FULL, "Application %s already in dictionary", new->data.application.application_name);
				/* got same id */
				if (fd_os_cmp(locref->data.application.application_name, locref->datastr_len, 
						new->data.application.application_name, new->datastr_len)) {
					TRACE_DEBUG(FULL, "Conflicting application name");
					break;
				}
				ret = 0;
				break;

			case DICT_TYPE:
				TRACE_DEBUG(FULL, "Type %s already in dictionary", new->data.type.type_name);
				/* got same name */
				if (locref->data.type.type_base != new->data.type.type_base) {
					TRACE_DEBUG(FULL, "Conflicting base type");
					break;
				}
				/* discard new definition only it a callback is provided and different from the previous one */
				if ((new->data.type.type_interpret) && (locref->data.type.type_interpret != new->data.type.type_interpret)) {
					TRACE_DEBUG(FULL, "Conflicting interpret cb");
					break;
				}
				if ((new->data.type.type_encode) && (locref->data.type.type_encode != new->data.type.type_encode)) {
					TRACE_DEBUG(FULL, "Conflicting encode cb");
					break;
				}
				if ((new->data.type.type_dump) && (locref->data.type.type_dump != new->data.type.type_dump)) {
					TRACE_DEBUG(FULL, "Conflicting dump cb");
					break;
				}
				ret = 0;
				break;

			case DICT_ENUMVAL:
				TRACE_DEBUG(FULL, "Enum %s already in dictionary", new->data.enumval.enum_name);
				/* got either same name or same value. We check that both are true */
				if (order_enum_by_name(locref, new)) {
					TRACE_DEBUG(FULL, "Conflicting enum name");
					break;
				}
				if (order_enum_by_val(locref, new)) {
					TRACE_DEBUG(FULL, "Conflicting enum value");
					break;
				}
				ret = 0;
				break;

			case DICT_AVP:
				TRACE_DEBUG(FULL, "AVP %s already in dictionary", new->data.avp.avp_name);
				/* got either same name or code */
				if (order_avp_by_code(locref, new)) {
					TRACE_DEBUG(FULL, "Conflicting AVP code");
					break;
				}
				if (order_avp_by_name(locref, new)) {
					TRACE_DEBUG(FULL, "Conflicting AVP name");
					break;
				}
				if  (locref->data.avp.avp_vendor != new->data.avp.avp_vendor) {
					TRACE_DEBUG(FULL, "Conflicting AVP vendor");
					break;
				}
				if  (locref->data.avp.avp_flag_mask != new->data.avp.avp_flag_mask) {
					TRACE_DEBUG(FULL, "Conflicting AVP flags mask");
					break;
				}
				if  ((locref->data.avp.avp_flag_val & locref->data.avp.avp_flag_mask) != (new->data.avp.avp_flag_val & new->data.avp.avp_flag_mask)) {
					TRACE_DEBUG(FULL, "Conflicting AVP flags value");
					break;
				}
				if  (locref->data.avp.avp_basetype != new->data.avp.avp_basetype) {
					TRACE_DEBUG(FULL, "Conflicting AVP base type");
					break;
				}
				ret = 0;
				break;

			case DICT_COMMAND:
				TRACE_DEBUG(FULL, "Command %s already in dictionary", new->data.cmd.cmd_name);
				/* We got either same name, or same code + R flag */
				if (order_cmd_by_name(locref, new)) {
					TRACE_DEBUG(FULL, "Conflicting command name");
					break;
				}
				if (locref->data.cmd.cmd_code != new->data.cmd.cmd_code) {
					TRACE_DEBUG(FULL, "Conflicting command code");
					break;
				}
				if (locref->data.cmd.cmd_flag_mask != new->data.cmd.cmd_flag_mask) {
					TRACE_DEBUG(FULL, "Conflicting command flags mask %hhx:%hhx", locref->data.cmd.cmd_flag_mask, new->data.cmd.cmd_flag_mask);
					break;
				}
				if ((locref->data.cmd.cmd_flag_val & locref->data.cmd.cmd_flag_mask) != (new->data.cmd.cmd_flag_val & new->data.cmd.cmd_flag_mask)) {
					TRACE_DEBUG(FULL, "Conflicting command flags value");
					break;
				}
				ret = 0;
				break;

			case DICT_RULE:
				/* Both rules point to the same AVPs (code & vendor) */
				if (locref->data.rule.rule_position != new->data.rule.rule_position) {
					TRACE_DEBUG(FULL, "Conflicting rule position");
					break;
				}
				if ( ((locref->data.rule.rule_position == RULE_FIXED_HEAD) ||
					(locref->data.rule.rule_position == RULE_FIXED_TAIL))
				    && (locref->data.rule.rule_order != new->data.rule.rule_order)) {
					TRACE_DEBUG(FULL, "Conflicting rule order");
					break;
				}
				if (locref->data.rule.rule_min != new->data.rule.rule_min) {
					int r1 = locref->data.rule.rule_min;
					int r2 = new->data.rule.rule_min;
					int p  = locref->data.rule.rule_position;
					if (  ((r1 != -1) && (r2 != -1)) /* none of the definitions contains the "default" value */
					   || ((p == RULE_OPTIONAL) && (r1 != 0) && (r2 != 0)) /* the other value is not 0 for an optional rule */
					   || ((r1 != 1) && (r2 != 1)) /* the other value is not 1 for another rule */
					) {
						TRACE_DEBUG(FULL, "Conflicting rule min");
						break;
					}
				}
				if (locref->data.rule.rule_max != new->data.rule.rule_max) {
					TRACE_DEBUG(FULL, "Conflicting rule max");
					break;
				}
				ret = 0;
				break;
		}
		if (!ret) {
			TRACE_DEBUG(FULL, "An existing object with the same data was found, ignoring the error...");
		}
		if (ref)
			*ref = locref;
	}
all_errors:
	if (ret != 0) {
		char * buf = NULL;
		size_t len = 0, offset=0;
		
		if (type == DICT_ENUMVAL) {
			CHECK_MALLOC( dump_enumval_data ( &buf, &len, &offset, data, parent->data.type.type_base ));
		} else {
			CHECK_MALLOC( dict_obj_info[CHECK_TYPE(type) ? type : 0].dump_data(&buf, &len, &offset, data) );
		}
	
		TRACE_DEBUG(INFO, "An error occurred while adding the following data in the dictionary: %s", buf);
		
		if (ret == EEXIST) {
			offset=0;
			CHECK_MALLOC( dump_object(&buf, &len, &offset, locref, 0, 0, 0) );
			TRACE_DEBUG(INFO, "Conflicting entry in the dictionary: %s", buf);
		}
		free(buf);
	}
error_free:
	free(new);
	return ret;
}


int fd_dict_delete(struct dict_object * obj)
{
	int i;
	struct dictionary * dict;
	int ret=0;
	
	/* check params */
	CHECK_PARAMS( verify_object(obj) && obj->dico);
	dict = obj->dico;

	/* Lock the dictionary for change */
#if ENABLE_LOCK_BYPASS
	if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX(  pthread_rwlock_wrlock(&dict->dict_lock)  );
	
	/* check the object is not sentinel for another list */
	for (i=0; i<NB_LISTS_PER_OBJ; i++) {
		if (!_OBINFO(obj).haslist[i] && !(FD_IS_LIST_EMPTY(&obj->list[i]))) {
			/* There are children, this is not good */
			ret = EINVAL;
			TRACE_DEBUG (FULL, "Cannot delete object, list %d not empty:", i);
			#if 0
			dump_list(&obj->list[i], 0,0,0);
			#endif
			break;
		}
	}
	
	/* ok, now destroy the object */
	if (!ret)
		destroy_object(obj);
	
	/* Unlock */
#if ENABLE_LOCK_BYPASS
	if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX(  pthread_rwlock_unlock(&dict->dict_lock)  );
	
	return ret;
}

void fd_dict_bypass_lock( struct dictionary *dict, int bypass )
{
#if ENABLE_LOCK_BYPASS
	if (dict && dict->dict_eyec == DICT_EYECATCHER)
		dict->dict_bypass_lock = bypass;
#endif
}

int fd_dict_search ( struct dictionary * dict, enum dict_object_type type, int criteria, const void * what, struct dict_object **result, int retval )
{
	int ret = 0;
	
	TRACE_ENTRY("%p %d(%s) %d %p %p %d", dict, type, dict_obj_info[CHECK_TYPE(type) ? type : 0].name, criteria, what, result, retval);
	
	/* Check param */
	CHECK_PARAMS( dict && (dict->dict_eyec == DICT_EYECATCHER) && CHECK_TYPE(type) );
	
	/* Lock the dictionary for reading */
#if ENABLE_LOCK_BYPASS
	if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX(  pthread_rwlock_rdlock(&dict->dict_lock)  );
	
	/* Now call the type-specific search function */
	ret = dict_obj_info[type].search_fct (dict, criteria, what, result);
	
	/* Unlock */
#if ENABLE_LOCK_BYPASS
	if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX(  pthread_rwlock_unlock(&dict->dict_lock)  );
	
	/* Update the return value as needed */
	if ((result != NULL) && (*result == NULL))
		ret = retval;
	
	return ret;
}

/* Function to retrieve list of objects in the dictionary. Use with care (read only).

All returned list must be accessed like this:

  for (li = sentinel->next; li != sentinel; li=li->next) {
	struct dict_object * obj = li->o;
	...
  }

The following criteria are allowed, with corresponding parent. 
The parent is either struct dictionary * or struct dict_object *
		
VENDOR_BY_ID : (parent = dictionary) returns list of vendors ordered by ID
APPLICATION_BY_ID : (parent = dictionary) returns list of applications ordered by ID
  ** for these two lists, the Vendor with id 0 and applciation with id 0 are excluded. 
     You must resolve them separatly with dict_search.
		
TYPE_BY_NAME : (parent = dictionary) returns list of types ordered by name (osstring order)
ENUMVAL_BY_NAME : (parent = type object) return list of constants for this type ordered by name (osstring order)
ENUMVAL_BY_VALUE : (parent = type object) return list of constants for this type ordered by values
AVP_BY_NAME : (parent = vendor object) return list of AVP for this vendor ordered by name (osstring order)
AVP_BY_CODE : (parent = vendor object) return list of AVP for this vendor ordered by code
CMD_BY_NAME : (parent = dictionary) returns list of commands ordered by name (osstring order)
CMD_BY_CODE_R : (parent = dictionary) returns list of commands ordered by code
RULE_BY_AVP_AND_PARENT: (parent = command or grouped AVP object) return list of rules for this object ordered by AVP vendor/code

All other criteria are rejected.
 */
int fd_dict_getlistof(int criteria, void * parent, struct fd_list ** sentinel)
{
	struct dictionary * dict = parent;
	struct dict_object * obj_parent = parent;
	
	TRACE_ENTRY("%i %p %p", criteria, parent, sentinel);
	
	CHECK_PARAMS(sentinel && parent);
	
	switch(criteria) {
		case VENDOR_BY_ID: /* parent must be the dictionary */
			CHECK_PARAMS(dict->dict_eyec == DICT_EYECATCHER);
			*sentinel = &dict->dict_vendors.list[0];
			break;
			
		case APPLICATION_BY_ID: /* parent must be the dictionary */
			CHECK_PARAMS(dict->dict_eyec == DICT_EYECATCHER);
			*sentinel = &dict->dict_applications.list[0];
			break;
			
		case TYPE_BY_NAME: /* parent must be the dictionary */
			CHECK_PARAMS(dict->dict_eyec == DICT_EYECATCHER);
			*sentinel = &dict->dict_types;
			break;
			
		case ENUMVAL_BY_NAME: /* parent must be a type object */
			CHECK_PARAMS(verify_object(obj_parent) && (obj_parent->type == DICT_TYPE));
			*sentinel = &obj_parent->list[1];
			break;
			
		case ENUMVAL_BY_VALUE: /* parent must be a type object */
			CHECK_PARAMS(verify_object(obj_parent) && (obj_parent->type == DICT_TYPE));
			*sentinel = &obj_parent->list[2];
			break;
			
		case AVP_BY_NAME: /* parent must be a VENDOR object */
			CHECK_PARAMS(verify_object(obj_parent) && (obj_parent->type == DICT_VENDOR));
			*sentinel = &obj_parent->list[2];
			break;
			
		case AVP_BY_CODE: /* parent must be a VENDOR object */
			CHECK_PARAMS(verify_object(obj_parent) && (obj_parent->type == DICT_VENDOR));
			*sentinel = &obj_parent->list[1];
			break;
			
		case CMD_BY_NAME: /* parent must be the dictionary */
			CHECK_PARAMS(dict->dict_eyec == DICT_EYECATCHER);
			*sentinel = &dict->dict_cmd_name;
			break;
			
		case CMD_BY_CODE_R: /* parent must be the dictionary */
			CHECK_PARAMS(dict->dict_eyec == DICT_EYECATCHER);
			*sentinel = &dict->dict_cmd_code;
			break;
			
		case RULE_BY_AVP_AND_PARENT: /* parent must be command or grouped AVP */
			CHECK_PARAMS(verify_object(obj_parent));
			CHECK_PARAMS(	(obj_parent->type == DICT_COMMAND) ||
					((obj_parent->type == DICT_AVP) 
					  && (obj_parent->data.avp.avp_basetype == AVP_TYPE_GROUPED)) );
			*sentinel = &obj_parent->list[2];
			break;
			
		default:
			CHECK_PARAMS(0);
	}
	
	return 0;
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/*                                                                                                     */
/*                                  The init/fini functions                                            */
/*                                                                                                     */
/*******************************************************************************************************/
/*******************************************************************************************************/

/* Initialize the dictionary */
int fd_dict_init ( struct dictionary ** dict)
{
	struct dictionary * new = NULL;
	
	TRACE_ENTRY("%p", dict);
	
	/* Sanity checks */
	ASSERT( (sizeof(type_base_name) / sizeof(type_base_name[0])) == (AVP_TYPE_MAX + 1) );
	ASSERT( (sizeof(dict_obj_info)  / sizeof(dict_obj_info[0]))  == (DICT_TYPE_MAX + 1) );
	CHECK_PARAMS(dict);
	
	/* Allocate the memory for the dictionary */
	CHECK_MALLOC( new = malloc(sizeof(struct dictionary)) );
	memset(new, 0, sizeof(struct dictionary));
	
	new->dict_eyec = DICT_EYECATCHER;
	
	/* Initialize the lock for the dictionary */
#if ENABLE_LOCK_BYPASS
	new->dict_bypass_lock = 0;
#endif
	CHECK_POSIX(  pthread_rwlock_init(&new->dict_lock, NULL)  );
	
	/* Initialize the sentinel for vendors and AVP lists */
	init_object( &new->dict_vendors, DICT_VENDOR );
	#define NO_VENDOR_NAME	"(no vendor)"
	new->dict_vendors.data.vendor.vendor_name = NO_VENDOR_NAME;
	new->dict_vendors.datastr_len = CONSTSTRLEN(NO_VENDOR_NAME);
	/* new->dict_vendors.list[0].o = NULL; *//* overwrite since element is also sentinel for this list. */
	new->dict_vendors.dico = new;
#if USE_HASHLIST
   initUInt32HashList(&new->dict_vendors.hashlist[0]);
   initStringHashList(&new->dict_vendors.hashlist[1]);
   LOG_N("HASHLIST is enabled");
#else
   LOG_N("HASHLIST is disabled");
#endif
	
	/* Initialize the sentinel for applications */
	init_object( &new->dict_applications, DICT_APPLICATION );
	#define APPLICATION_0_NAME	"Diameter Common Messages"
	new->dict_applications.data.application.application_name = APPLICATION_0_NAME;
	new->dict_applications.datastr_len = CONSTSTRLEN(APPLICATION_0_NAME);
	/* new->dict_applications.list[0].o = NULL; *//* overwrite since since element is also sentinel for this list. */
	new->dict_applications.dico = new;
			
	/* Initialize the sentinel for types */
	fd_list_init ( &new->dict_types, NULL );
	
	/* Initialize the sentinels for commands */
	fd_list_init ( &new->dict_cmd_name, NULL );
	fd_list_init ( &new->dict_cmd_code, NULL );
	
	/* Initialize the error command object */
	init_object( &new->dict_cmd_error, DICT_COMMAND );
	#define GENERIC_ERROR_NAME	"(generic error format)"
	new->dict_cmd_error.data.cmd.cmd_name = GENERIC_ERROR_NAME;
	new->dict_cmd_error.datastr_len = CONSTSTRLEN(GENERIC_ERROR_NAME);
	new->dict_cmd_error.data.cmd.cmd_flag_mask=CMD_FLAG_ERROR | CMD_FLAG_REQUEST | CMD_FLAG_RETRANSMIT;
	new->dict_cmd_error.data.cmd.cmd_flag_val =CMD_FLAG_ERROR;
	new->dict_cmd_error.dico = new;
	
	*dict = new;
	
	/* Done */
	return 0;
}

/* Destroy a dictionary */
int fd_dict_fini ( struct dictionary ** dict)
{
	int i;
	
	TRACE_ENTRY("");
	CHECK_PARAMS( dict && *dict && ((*dict)->dict_eyec == DICT_EYECATCHER) );
	
	/* Acquire the write lock to make sure no other operation is ongoing */
#if ENABLE_LOCK_BYPASS
	if (!(*dict)->dict_bypass_lock)
#endif
	CHECK_POSIX(  pthread_rwlock_wrlock(&(*dict)->dict_lock)  );
	
	/* Empty all the lists, free the elements */
	destroy_list ( &(*dict)->dict_cmd_error.list[2] );
	destroy_list ( &(*dict)->dict_cmd_code );
	destroy_list ( &(*dict)->dict_cmd_name );
	destroy_list ( &(*dict)->dict_types );
	for (i=0; i< NB_LISTS_PER_OBJ; i++) {
		destroy_list ( &(*dict)->dict_applications.list[i] );
		destroy_list ( &(*dict)->dict_vendors.list[i] );
	}
	
	/* Dictionary is empty, now destroy the lock */
#if ENABLE_LOCK_BYPASS
	if (!(*dict)->dict_bypass_lock)
#endif
	CHECK_POSIX(  pthread_rwlock_unlock(&(*dict)->dict_lock)  );
	CHECK_POSIX(  pthread_rwlock_destroy(&(*dict)->dict_lock)  );
	
	free(*dict);
	*dict = NULL;
	
	return 0;
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/*                                                                                                     */
/*                                  Other functions                                                    */
/*                                                                                                     */
/*******************************************************************************************************/
/*******************************************************************************************************/

/* Iterate a callback on the rules for an object */
int fd_dict_iterate_rules ( struct dict_object *parent, void * data, int (*cb)(void *, struct dict_rule_data *) )
{
	int ret = 0;
	struct fd_list * li;
	
	TRACE_ENTRY("%p %p %p", parent, data, cb);
	
	/* Check parameters */
	CHECK_PARAMS(  verify_object(parent)  );
	CHECK_PARAMS(  (parent->type == DICT_COMMAND) 
			|| ((parent->type == DICT_AVP) && (parent->data.avp.avp_basetype == AVP_TYPE_GROUPED)) );
	TRACE_DEBUG (FULL, "Iterating on rules of %s: '%s'.", 
			_OBINFO(parent).name, 
			parent->type == DICT_COMMAND ? 
				  parent->data.cmd.cmd_name
				: parent->data.avp.avp_name);
	
	/* Acquire the read lock  */
#if ENABLE_LOCK_BYPASS
	if (!parent->dico->dict_bypass_lock)
#endif
	CHECK_POSIX(  pthread_rwlock_rdlock(&parent->dico->dict_lock)  );
	
	/* go through the list and call the cb on each rule data */
	for (li = &(parent->list[2]); li->next != &(parent->list[2]); li = li->next) {
		ret = (*cb)(data, &(_O(li->next->o)->data.rule));
		if (ret != 0)
			break;
	}
		
	/* Release the lock */
#if ENABLE_LOCK_BYPASS
	if (!parent->dico->dict_bypass_lock)
#endif
	CHECK_POSIX(  pthread_rwlock_unlock(&parent->dico->dict_lock)  );
	
	return ret;
}

/* Create the list of vendors. Returns a 0-terminated array, that must be freed after use. Returns NULL on error. */
uint32_t * fd_dict_get_vendorid_list(struct dictionary * dict)
{
	uint32_t * ret = NULL;
	int i = 0;
	struct fd_list * li;
	
	TRACE_ENTRY();
	
	/* Acquire the read lock */
#if ENABLE_LOCK_BYPASS
	if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX_DO(  pthread_rwlock_rdlock(&dict->dict_lock), return NULL  );
	
	/* Allocate an array to contain all the elements */
	CHECK_MALLOC_DO( ret = calloc( dict->dict_count[DICT_VENDOR] + 1, sizeof(uint32_t) ), goto out );
	
	/* Copy the vendors IDs */
	for (li = dict->dict_vendors.list[0].next; li != &(dict->dict_vendors.list[0]); li = li->next) {
		ret[i] = _O(li->o)->data.vendor.vendor_id;
		i++;
		ASSERT( i <= dict->dict_count[DICT_VENDOR] );
	}
out:	
	/* Release the lock */
#if ENABLE_LOCK_BYPASS
   if (!dict->dict_bypass_lock)
#endif
	CHECK_POSIX_DO(  pthread_rwlock_unlock(&dict->dict_lock), return NULL  );
	
	return ret;
}

/* Return the location of the cb list for an object, after checking its type */
int fd_dict_disp_cb(enum dict_object_type type, struct dict_object *obj, struct fd_list ** cb_list)
{
	TRACE_ENTRY("%d %p %p", type, obj, cb_list);
	CHECK_PARAMS( verify_object(obj) );
	CHECK_PARAMS( _OBINFO(obj).type == type );
	CHECK_PARAMS( cb_list );
	*cb_list = &obj->disp_cbs;
	return 0;
}

int fd_dict_get_error_cmd(struct dictionary * dict, struct dict_object **obj)
{
	TRACE_ENTRY("%p %p", dict, obj);
	CHECK_PARAMS( dict && (dict->dict_eyec == DICT_EYECATCHER) && obj );
	*obj = &dict->dict_cmd_error;
	return 0;
}
