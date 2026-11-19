#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>
#include <gint/defs/timeout.h>
#include <justui/jbutton.h>
#include <justui/jlabel.h>
#include <justui/jlayout.h>
#include <justui/jscene.h>
#include <justui/jwidget.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHUNK (512u*1024u)
#define SLICE 2048u
#define MAX_SIZE (32u*1024u*1024u)
static usb_interface_t const *interfaces[]={&usb_ff_bulk,NULL};
static jscene *scene;
static jlabel *status;
static char message[100];

static void display(char const *s)
{
    if(s != message) snprintf(message,sizeof message,"%s",s);
    jlabel_set_text(status,message);
    dclear(C_WHITE);
    jscene_render(scene);
    dupdate();
}
static uint32_t le32(uint8_t const *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
           ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static void write32(uint8_t *p,uint32_t v)
{
    p[0]=v;p[1]=v>>8;p[2]=v>>16;p[3]=v>>24;
}
static int send_chunk(uint32_t address,uint32_t amount)
{
    usb_fxlink_header_t h;
    uint8_t info[8];
    int pipe=usb_ff_bulk_output();
    if(pipe<0) return -1;
    if(!usb_fxlink_fill_header(&h,"ramdump","data",amount+8)) return -2;
    write32(info,address);
    write32(info+4,amount);
    timeout_t t=timeout_make_ms(3000);
    if(usb_write_sync_timeout(pipe,&h,sizeof h,false,&t)<0) return -3;
    t=timeout_make_ms(3000);
    if(usb_write_sync_timeout(pipe,info,8,false,&t)<0) return -4;
    for(uint32_t pos=0;pos<amount;pos+=SLICE) {
        uint32_t n=amount-pos;
        if(n>SLICE) n=SLICE;
        void const *src=(void const *)(uintptr_t)(address+pos);
        t=timeout_make_ms(3000);
        if(usb_write_sync_timeout(pipe,src,n,false,&t)<0) return -5;
    }
    t=timeout_make_ms(3000);
    return usb_commit_sync_timeout(pipe,&t)<0 ? -6 : 0;
}
static int dump(void)
{
    int rc=usb_open(interfaces,GINT_CALL_NULL);
    if(rc) return rc;
    uint32_t start=0,size=0;
    display("Waiting for Windows receiver range...");
    for(;;) {
        if(usb_is_open()) {
            usb_fxlink_header_t h;
            if(usb_fxlink_handle_messages(&h)) {
                if(!strncmp(h.application,"ramdump",16) &&
                   !strncmp(h.type,"start",16) && h.size==8) {
                    uint8_t request[8];
                    timeout_t t=timeout_make_ms(2000);
                    int n=usb_read_sync_timeout(usb_ff_bulk_input(),
                                                request,8,false,&t);
                    if(n==8) {
                        uint32_t first=le32(request),last=le32(request+4);
                        uint64_t length=(uint64_t)last-first+1;
                        if(first>=0x80000000u && last<=0x9fffffffu &&
                           last>=first && length<=MAX_SIZE) {
                            start=first;size=(uint32_t)length;
                            break;
                        }
                        display("Invalid host range (max 32 MiB)");
                    }
                }
                usb_fxlink_drop_transaction();
            }
        }
        key_event_t e=pollevent();
        if(e.type==KEYEV_DOWN &&
           (e.key==KEY_CLEAR || e.key==KEY_HOME || e.key==KEY_EXIT)) {
            usb_close();
            return -10;
        }
    }
    snprintf(message,sizeof message,"Reading %08lX..%08lX",
             (unsigned long)start,(unsigned long)(start+size-1));
    display(message);
    for(uint32_t offset=0;offset<size;offset+=CHUNK) {
        uint32_t amount=size-offset;
        if(amount>CHUNK) amount=CHUNK;
        rc=send_chunk(start+offset,amount);
        if(rc) {usb_close();return rc;}
        uint32_t done=offset+amount;
        snprintf(message,sizeof message,"%lu / %lu KiB  (%lu%%)",
                 (unsigned long)(done/1024),
                 (unsigned long)((size+1023)/1024),
                 (unsigned long)(((uint64_t)done*100)/size));
        display(message);
    }
    usb_close();
    return 0;
}
int main(void)
{
    scene=jscene_create_fullscreen(NULL);
    jlayout_set_vbox(scene)->spacing=14;
    jwidget_set_padding(scene,18,18,18,18);
    jlabel_create("ClassPad RAM -> USB",scene);
    jlabel_create("Host-defined range (max 32 MiB)",scene);
    status=jlabel_create("Read-only. Unmapped addresses can crash.",scene);
    jbutton *start=jbutton_create("Start dump",scene);
    jbutton *exit_button=jbutton_create("Exit",scene);
    jscene_show_and_focus(scene,start);
    for(;;) {
        jevent e=jscene_run(scene);
        if(e.type==JSCENE_PAINT) {
            dclear(C_WHITE);jscene_render(scene);dupdate();
        }
        else if(e.type==JBUTTON_TRIGGERED && e.source==exit_button) break;
        else if(e.type==JBUTTON_TRIGGERED && e.source==start) {
            jbutton_set_disabled(start,true);
            display("Opening USB...");
            int rc=dump();
            if(rc) {
                snprintf(message,sizeof message,"Stopped / error %d",rc);
                display(message);
            }
            else display("Dump complete");
            jbutton_set_disabled(start,false);
        }
    }
    jwidget_destroy(scene);
    return 1;
}
