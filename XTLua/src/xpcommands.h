//
//  xpcommands.h
//  xlua
//
//  Created by Benjamin Supnik on 4/12/16.
//
//	Copyright 2016, Laminar Research
//	This source code is licensed under the MIT open source license.
//	See LICENSE.txt for the full terms of the license.

#ifndef xpcommands_h
#define xpcommands_h

struct	xtlua_cmd;
struct	xlua_cmd;
typedef void (* xtlua_cmd_handler_f)(xtlua_cmd * cmd, int phase, float duration, void * ref);
typedef void (* xlua_cmd_handler_f)(xlua_cmd * cmd, int phase, float duration, void * ref);
xtlua_cmd * xtlua_find_cmd(const char * name);
xlua_cmd * xlua_find_cmd(const char * name);
xlua_cmd * xlua_create_cmd(const char * name, const char * desc);

// The main handler can be used to provide guts to our command or REPLACE an existing
// command. The pre/post handlers always augment.
void xtlua_cmd_install_handler(xtlua_cmd * cmd, xtlua_cmd_handler_f handler, void * ref);
void xlua_cmd_install_handler(xlua_cmd * cmd, xlua_cmd_handler_f handler, void * ref);
void xtlua_cmd_install_pre_wrapper(xtlua_cmd * cmd, xtlua_cmd_handler_f handler, void * ref);
void xtlua_cmd_install_post_wrapper(xtlua_cmd * cmd, xtlua_cmd_handler_f handler, void * ref);

void xtlua_cmd_start(xtlua_cmd * cmd);
void xtlua_cmd_stop(xtlua_cmd * cmd);
void xtlua_cmd_once(xtlua_cmd * cmd);
//void xlua_std_pre_cmd(xtlua_cmd * cmd,int phase);
void xtlua_cmd_cleanup();

#endif /* xpcommands_h */
