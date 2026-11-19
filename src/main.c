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

/* First 32 MiB of the requested 0x88000000 mapping (not RAM detection). */
#define RAM_START 0x88000000u
#define RAM_SIZE  0x02000000u
#define MESSAGE_SIZE (512u * 1024u)
#define USB_SLICE 2048u

static usb_interface_t const *interfaces[] = { &usb_ff_bulk, NULL };
static char progress_text[80];
static jlabel *progress;
static jscene *scene;

static void show(char const *message)
{
    snprintf(progress_text, sizeof progress_text, "%s", message);
    jlabel_set_text(progress, progress_text);
    dclear(C_WHITE);
    jscene_render(scene);
    dupdate();
}

static void put_le32(uint8_t *p, uint32_t v)
{
    p[0] = v;
    p[1] = v >> 8;
    p[2] = v >> 16;
    p[3] = v >> 24;
}

/* A 512 KiB fxlink MESSAGE, broken into small USB writes; no large buffer. */
static int send_chunk(uint32_t offset)
{
    usb_fxlink_header_t hdr;
    uint8_t descriptor[8];
    int pipe = usb_ff_bulk_output();
    if(pipe < 0) return -1;
    if(!usb_fxlink_fill_header(&hdr, "ramdump", "data",
                               8u + MESSAGE_SIZE)) return -2;
    put_le32(descriptor, offset);
    put_le32(descriptor + 4, MESSAGE_SIZE);

    timeout_t deadline = timeout_make_ms(3000);
    if(usb_write_sync_timeout(pipe, &hdr, sizeof hdr, false, &deadline) < 0)
        return -3;
    deadline = timeout_make_ms(3000);
    if(usb_write_sync_timeout(pipe, descriptor, sizeof descriptor,
                              false, &deadline) < 0) return -4;

    /* Direct reads; assumes the entire requested range is readable. */
    for(uint32_t pos=0; pos<MESSAGE_SIZE; pos+=USB_SLICE) {
        void const *source = (void const *)(uintptr_t)(RAM_START + offset + pos);
        deadline = timeout_make_ms(3000);
        if(usb_write_sync_timeout(pipe, source, USB_SLICE,
                                  false, &deadline) < 0) return -5;
    }
    deadline = timeout_make_ms(3000);
    return usb_commit_sync_timeout(pipe, &deadline) < 0 ? -6 : 0;
}

static int dump(void)
{
    int rc = usb_open(interfaces, GINT_CALL_NULL);
    if(rc) return rc;

    show("Connect USB, then start the Windows receiver");
    for(;;) {
        if(usb_is_open()) {
            usb_fxlink_header_t hdr;
            if(usb_fxlink_handle_messages(&hdr)) {
                if(!strncmp(hdr.application, "ramdump", 16) &&
                   !strncmp(hdr.type, "start", 16) && hdr.size==4) {
                    uint8_t go[4];
                    timeout_t deadline=timeout_make_ms(2000);
                    int n=usb_read_sync_timeout(usb_ff_bulk_input(),
                                                go, 4, false, &deadline);
                    if(n==4 && !memcmp(go, "GO32", 4)) break;
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

    for(uint32_t offset=0; offset<RAM_SIZE; offset+=MESSAGE_SIZE) {
        rc=send_chunk(offset);
        if(rc) {
            usb_close();
            return rc;
        }
        unsigned done=(offset+MESSAGE_SIZE) / (1024u*1024u);
        snprintf(progress_text, sizeof progress_text,
                 "%u / 32 MiB (%u%%)", done, done*100/32);
        jlabel_set_text(progress, progress_text);
        dclear(C_WHITE);
        jscene_render(scene);
        dupdate();
    }
    usb_close();
    return 0;
}

int main(void)
{
    scene=jscene_create_fullscreen(NULL);
    jlayout_set_vbox(scene)->spacing=14;
    jwidget_set_padding(scene, 18, 18, 18, 18);
    jlabel_create("ClassPad RAM -> USB", scene);
    jlabel_create("0x88000000 - 0x89FFFFFF (32 MiB)", scene);
    progress=jlabel_create("Press Start. Read-only; may fault on unmapped RAM.",scene);
    jbutton *start=jbutton_create("Start dump", scene);
    jbutton *exit_button=jbutton_create("Exit", scene);
    jscene_show_and_focus(scene, start);

    for(;;) {
        jevent e=jscene_run(scene);
        if(e.type==JSCENE_PAINT) {
            dclear(C_WHITE);
            jscene_render(scene);
            dupdate();
        }
        else if(e.type==JBUTTON_TRIGGERED && e.source==exit_button)
            break;
        else if(e.type==JBUTTON_TRIGGERED && e.source==start) {
            jbutton_set_disabled(start, true);
            show("Opening USB...");
            int rc=dump();
            if(rc) {
                snprintf(progress_text, sizeof progress_text,
                         "Stopped / USB error %d", rc);
                jlabel_set_text(progress, progress_text);
            }
            else show("Done: 32 MiB sent");
            jbutton_set_disabled(start, false);
        }
    }
    jwidget_destroy(scene);
    return 1;
}
