/*
 * $Id$
 *
 *  captagent - Homer capture agent. Modular
 *  Duplicate SIP messages in Homer Encapulate Protocol [HEP] [ipv6 version]
 *
 *  Author: Alexandr Dubovikov <alexandr.dubovikov@gmail.com>
 *  (C) Homer Project 2012 (http://www.sipcapture.org)
 *
 * Homer capture agent is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version
 *
 * Homer capture agent is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "api.h"
#include "log.h"
#include "xmlread.h"
#include "modules.h"


#include "captagent.h"


char *dupArgs[2];
extern char *config_file;
char *server;
xml_node *tree;

void handler(int value)
{

	int terminating = 1;

        LNOTICE( "The agent has been terminated\n");

        if (pid_file) unlink(pid_file);        

        /* now we are free */
        xml_free( tree );        

        /* HEPMODE */
        if(hepmod) free(hepmod);
        
        /* destroy LOG */
        destroy_log();

        if(!unregister_modules()) {
        	LNOTICE("DONE unload\n");
        }

        exit(0);
}


int send_message (rc_info_t *rcinfo, unsigned char *data, unsigned int len) {

        int res;        
	if(hepmod->send_hep_basic) {		    
            if (!(res = hepmod->send_hep_basic(rcinfo, data, len))) {
                    LERR( "not send, returning %d\n", res);
                    return -1;
            }        		
        }

        return 1;
}

int get_basestat (char *module, char *buf) {

        char *res;      
        int pos = 0;
        char stat[1024];        

	struct module *m = NULL;
        m = module_list;
        while(m) {

                if(!strncmp(module, "all", 3)) {
                        if(m->statistic(stat)) {
                                pos += snprintf(buf+pos, MAX_STATS - pos, "%s\r\n", stat);
                        }
                }
                else {
                        if(!strncmp(m->resource, module, strlen(module))) {
                                if(m->statistic(stat)) {
                                      pos += snprintf(buf+pos, MAX_STATS - pos, "%s\r\n", stat);
                                }
                                return pos;
                        }                
                }
                
                m = m->next;
        }                
        
        return pos;
}


int daemonize(int nofork)
{

        FILE *pid_stream;
        pid_t pid;
        int p;
        struct sigaction new_action;


         if (!nofork) {

                if ((pid=fork())<0){
                        LERR("Cannot fork:%s\n", strerror(errno));
                        goto error;
                }else if (pid!=0){
                        exit(0);
                }
        }

        if (pid_file!=0){
                if ((pid_stream=fopen(pid_file, "r"))!=NULL){
                        if (fscanf(pid_stream, "%d", &p) < 0) {
                                LERR("could not parse pid file %s\n", pid_file);
                        }
                        fclose(pid_stream);
                        if (p==-1){
                                LERR("pid file %s exists, but doesn't contain a valid"
                                        " pid number\n", pid_file);
                                goto error;
                        }
                        if (kill((pid_t)p, 0)==0 || errno==EPERM){
                                LERR("running process found in the pid file %s\n",
                                        pid_file);
                                goto error;
                        }else{
                               LERR("pid file contains old pid, replacing pid\n");
                        }
                }
                pid=getpid();
                if ((pid_stream=fopen(pid_file, "w"))==NULL){
                        LERR("unable to create pid file %s: %s\n", pid_file, strerror(errno));
                        goto error;
                }else{
                        fprintf(pid_stream, "%i\n", (int)pid);
                        fclose(pid_stream);
                }
        }

        /* sigation structure */
        new_action.sa_handler = handler;
        sigemptyset (&new_action.sa_mask);
        new_action.sa_flags = 0;

        if( sigaction (SIGINT, &new_action, NULL) == -1) {
                perror("Failed to set new Handle");
                return -1;
        }
        if( sigaction (SIGTERM, &new_action, NULL) == -1) {
                perror("Failed to set new Handle");
                return -1;
        }

        return 0;
error:
        return -1;

}

xml_node *get_module_config( const char *mod_name, xml_node *mytree) {

    xml_node *next, *modules = NULL, *config;
    int ret = 0, i = 0;
    char cfg[128];
    
    ret = snprintf(cfg, sizeof(cfg), "%s.conf", mod_name);
         
    next = mytree;

    while(1) {
    
        if(next == NULL) break;        
        next = xml_get("configuration", next, 1 );                    
        for(i=0;next->attr[i];i++) {    
            if(!strncmp(next->attr[i], "name", 4)) {                        
                if(!strncmp(next->attr[i+1], cfg, ret)) {
                        modules =  next;
                        break;
                }                            
            }
        }            
        next = next->next;        
    }    
    return modules;
}

void usage(int8_t e) {
    printf("usage: captagent <-vh> <-f config>\n"
           "   -h  is help/usage\n"
           "   -v  is version information\n"
           "   -f  is the config file\n"
           "   -D  is use specified pcap file instead of a device from the config\n"
           "   -c  is checkout\n"
           "   -d  is daemon mode\n"
           "   -n  is foreground mode\n"
           "");
        exit(e);
}


int main( int argc, char *argv[] ) {

    xml_node *next, *modules, *config;
    const char *file = DEFAULT_CONFIG;
    const char **attr, **attr_mod;
    int i = 0, y = 0, c, checkout = 0;

    while((c=getopt(argc, argv, "dcvnhf:D:"))!=EOF) {
                switch(c) {
                        case 'v':
				printf("version: %s\n", VERSION);
				exit(0);
				break;
			case 'f':
				file = optarg;
                                break;
			case 'd':
				nofork = 0;
                                break;                                
                        case '?':
                        case 'h':
                        	usage(0);
                                break;
			case 'c':
                        	checkout = 1;
                                break;      
                        case 'D':
                                usefile = optarg;
                                break;                         
			case 'n':
				foreground = 1;
                                break;                                                                
                                
			default:
                                abort();

		}
    }

    hepmod = malloc(sizeof(hep_module_t));

    if( (tree = xml_parse( file )) == NULL ) {
          LERR( "Unable to open configuration file: %s\n", file );
          exit( 1 );
    }


    /* PATH */
    module_path = MODULE_DIR;
      
    /*CORE CONFIG */    

    if(!(config = get_module_config("core", tree))) {
        LERR( "Config for core has been not found\n");
    }           
    else {
            if(!core_config(tree)) {
                    LERR( "Config for core found\n");
            }    
    }
    
    if(foreground) nofork = 1;
    
    if(daemonize(nofork) != 0){
                LERR("Daemoniize failed: %s\n", strerror(errno));
                exit(-1);
    }
        
    next = tree;
                                   
    while(1) {
    
        if(next == NULL) break;
        
        next = xml_get("configuration", next, 1 );                    

        for(i=0;next->attr[i];i++) {    
            if(!strncmp(next->attr[i], "name", 4)) {
                        
                if(!strncmp(next->attr[i+1], "modules.conf", 13)) {
                        modules =  next;                                                                
                        while(1) {      
                            if(modules ==  NULL) break;
                            modules = xml_get("load", modules, 1 );
                            if(modules->attr[0] != NULL && modules->attr[1] != NULL ) {
                                    /* get config */
                                    if(!(config = get_module_config(modules->attr[1], tree))) {
                                            LERR( "Config for [%s] has been not found\n", modules->attr[1]);
                                    }                                                                                                            
                                    if(!register_module(modules->attr[1], config)) {
                                            LERR( "Module [%s] couldnot be registered\n", modules->attr[1]);
                                    }
                            }                                                        
                            
                            modules = modules->next;                            
                        }
                }                            
            }
        }
            
        next = next->next;
        
    }
        
    

    LNOTICE("The Captagent is ready\n");
    select(0,NULL,NULL,NULL,NULL);
    
    return 0;
                    
}


int core_config (xml_node *config)
{
        char *dev, *usedev = NULL;
        xml_node *modules;
        char *key, *value;
        int _use_syslog = 0;
                

        init_log("captagent", 1);

        LNOTICE("Loaded core config\n");

        /* READ CONFIG */
        modules = config;

        while(1) {
                if(modules ==  NULL) break;
                modules = xml_get("param", modules, 1 );
                if(modules->attr[0] != NULL && modules->attr[2] != NULL) {

                        /* bad parser */
                        if(strncmp(modules->attr[2], "value", 5) || strncmp(modules->attr[0], "name", 4)) {
                            LERR( "bad keys in the config\n");
                            goto next;

                        }

                        key =  modules->attr[1];
                        value = modules->attr[3];

                        if(key == NULL || value == NULL) {
                            LERR( "bad values in the config\n");
                            goto next;

                        }

                        if(!strncmp(key, "debug", 5)) debug_level = atoi(value);
                        else if(!strncmp(key, "daemon", 6) && !strncmp(value, "true", 5) && nofork == 1) nofork = 0;
                        else if (!strncmp(key, "syslog", 6) && !strncmp(value, "true", 5)) _use_syslog = 1;
                        else if(!strncmp(key, "path", 4)) module_path = value;
                        else if(!strncmp(key, "pid_file", 8)) pid_file = value;
                }
next:

                modules = modules->next;
        }
        
        
        set_log_level(debug_level);

	if (_use_syslog == 0) {

		destroy_log();
		init_log("captagent", 0);
	}

	return 1;
}
