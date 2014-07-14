
#ifndef _RIPOSTE_SYMBOLS_H
#define _RIPOSTE_SYMBOLS_H

#include "common.h"
#include "gc.h"

// predefined strings

#define STRINGS(_) 			        \
	/* Type names */ 		        \
	_(Nil,		    "nil")		    \
	_(Promise, 	    "promise") 	    \
	_(Future,	    "future")		\
	_(Closure,	    "closure")	    \
	_(Environment,	"environment")	\
	_(Externalptr,	"externalptr")	\
	_(Null, 	    "NULL")		    \
	_(Raw, 		    "raw")		    \
	_(Logical, 	    "logical")	    \
	_(Integer, 	    "integer")	    \
	_(Double, 	    "double")	    \
	_(Character, 	"character")	\
	_(List,		    "list")		    \
	/* Constant strings */	        \
	_(empty, 	    "")             \
	_(NArep, 	    "<NA>")         \
	_(dots, 	    "...")          \
    _(assignTmp,    "*tmp*")        \
	_(Maximal,      "\255")         \
    /* Primitive functions */ \
	_(isnil,        "is.nil")   \
    _(Complex, 	    "complex")	\
	_(Function,	    "function")	\
	_(BuiltIn,	    "builtin")	\
	_(Object,	    "object")	\
	_(Default, 	    "default") 	\
	_(Dotdot, 	    "dotdot") 	\
	_(Date,         "date")		\
	_(numeric, 	    "numeric") \
	_(pairlist,	    "pairlist")	\
	_(name,	        "name")	\
	_(call,	        "call")		\
	_(expression,	"expression")	\
	_(external, 	".External") \
	_(map, 	        ".Map") \
	_(scan, 	    ".Scan") \
	_(fold, 	    ".Fold") \
	_(assign, 	"<-") \
	_(assign2, 	"<<-") \
	_(eqassign, 	"=") \
    _(rm,           "rm") \
	_(function, 	"function") \
	_(returnSym, 	"return") \
	_(forSym, 	"for") \
	_(whileSym, 	"while") \
	_(repeatSym, 	"repeat") \
	_(nextSym, 	"next") \
	_(breakSym,	"break") \
	_(ifSym, 	"if") \
	_(switchSym, 	"switch") \
	_(brace, 	"{") \
	_(paren, 	"(") \
	_(pos, 		"+") \
	_(add, 		"+") \
	_(neg, 		"-") \
	_(sub, 		"-") \
	_(mul, 		"*") \
	_(tilde, 	"~") \
	_(div, 		"/") \
	_(idiv, 	"%/%") \
	_(pow, 		"^") \
	_(mod, 		"%%") \
	_(atan2, 	"atan2") \
	_(hypot, 	"hypot") \
	_(lnot, 	"!") \
	_(land, 	"&") \
	_(lor, 		"|") \
	_(land2, 	"&&") \
	_(lor2, 	"||") \
	_(eq, 		"==") \
	_(neq, 		"!=") \
	_(lt, 		"<") \
	_(le, 		"<=") \
	_(gt, 		">") \
	_(ge, 		">=") \
	_(dollar, 	"$") \
	_(at, 		"@") \
	_(colon, 	":") \
	_(nsget, 	"::") \
	_(nsgetint, 	":::") \
	_(bracket, 	"[") \
	_(bb, 		"[[") \
	_(question, 	"?") \
	_(round, 	"round") \
	_(signif, 	"signif") \
	_(sum,		"sum") \
	_(prod,		"prod") \
	_(mean,		"mean") \
	_(cm2,		"cm2") \
	_(min,		"min") \
	_(max,		"max") \
	_(pmin,		"pmin") \
	_(pmax,		"pmax") \
	_(any,		"any") \
	_(all,		"all") \
	_(cumsum,	"cumsum") \
	_(cumprod,	"cumprod") \
	_(cummin,	"cummin") \
	_(cummax,	"cummax") \
	_(cumany,	"cumany") \
	_(cumall,	"cumall") \
	_(names, 	"names") \
	_(dim, 		"dim") \
	_(classSym, 	"class") \
	_(bracketAssign, "[<-") \
	_(bbAssign, 	"[[<-") \
	_(dollarAssign, "$<-") \
	_(UseMethod, 	"UseMethod") \
	_(seqlen, 		"seq_len") \
	_(index, 	"index") \
	_(random, 	"random") \
	_(True, 	"TRUE") \
	_(False, 	"FALSE") \
	_(type, 	".type") \
	_(length, 	"length") \
	_(value, 	"value") \
	_(dotGeneric, 	".Generic") \
	_(dotMethod, 	".Method") \
	_(dotClass, 	".Class") \
	_(docall,	"do.call") \
	_(list,		"list") \
	_(missing,	"missing") \
	_(mmul,		"%*%") \
	_(apply,	"apply") \
	_(ifelse,	"ifelse") \
	_(split,	"split") \
	_(strip,	"strip") \
	_(isna,		"is.na") \
	_(vector,	"vector") \
	_(attrget,	"attr") \
	_(attrset,	"attr<-") \
	_(Re,	"Re") \
	_(Im,	"Im") \
    _(as, "as") \
    _(promise, "promise") \
    _(getenv, ".getenv") \
    _(setenv, ".setenv") \
    _(body, "body") \
    _(formals, "formals") \
    _(attributes, "attributes") \
    _(pr_expr, ".pr_expr") \
    _(pr_env, ".pr_env") \
    _(env_new, ".env_new") \
    _(env_names, ".env_names") \
    _(env_global, ".env_global") \
    _(env_remove, ".env_remove") \
    _(env_get, ".get") \
    _(frame, ".frame") \
    _(top, ".top") \
    _(print, "print") \
    _(Last_value, ".Last.value") \
    _(semijoin, ".semijoin") \
    _(stop, ".stop") \
    _(invisible, ".invisible") \
    _(visible, ".visible") \
    _(__onexit__, ".__on.exit__.") \
    _(__extraArgs__, ".__extraArgs__.") \
    _(__nargs__, ".__nargs__.") \
    _(__function__, ".__function__.") \
    _(__call__, ".__call__.") \
    _(__parent__, ".__parent__.") \
    _(__dots__, ".__dots__.") \
    _(__names__, ".__names__.") \
    _(rownames, "row.names") \

struct StringImpl : public HeapObject {
    char s[];
};
typedef const StringImpl* String;

//typedef const char* String;

namespace Strings {
	static const String NA = 0;
#define DECLARE(name, string, ...) extern String name;
	STRINGS(DECLARE)
#undef DECLARE
}

#endif

