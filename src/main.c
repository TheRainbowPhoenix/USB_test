#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>
#include <justui/jbutton.h>
#include <justui/jlabel.h>
#include <justui/jlayout.h>
#include <justui/jscene.h>
#include <justui/jwidget.h>
#include <stdio.h>
#include <string.h>

// USB Interfaces
static usb_interface_t const *interfaces[] = {&usb_ff_bulk, NULL};

// USB Test Data
static usb_fxlink_header_t header_text;
static usb_fxlink_header_t header_image;
static char const *short_text = "Hello JustUI!";
static char read_buffer[256]; // Buffer for receiving data

int main(void)
{
  // Prepare USB headers
  usb_fxlink_fill_header(&header_text, "justui", "text", strlen(short_text));

  // Correctly calculate VRAM size
  // DWIDTH and DHEIGHT are from gint/display.h
  int vram_size = DWIDTH * DHEIGHT * sizeof(color_t);
  usb_fxlink_fill_header(&header_image, "justui", "image", vram_size);

  // Create the main scene (fullscreen)
  jscene *scene = jscene_create_fullscreen(NULL);

  // We will use a main vertical box layout:
  // Top component will be a stack layout holding our tabs.
  // Bottom component will be a horizontal box holding navigation buttons.
  jlayout_set_vbox(scene)->spacing = 0;
  // ==========================================
  // STACKED TABS
  // ==========================================
  // The main area that actually holds the different tabs
  jwidget *stack = jwidget_create(scene);
  jlayout_set_stack(stack);
  // Stretch to take all remaining vertical space
  jwidget_set_stretch(stack, 1, 1, false);

  // ------------- TAB 1: Welcome -------------
  jwidget *tab1 = jwidget_create(stack);
  jlayout_set_vbox(tab1)->spacing = 10;
  // Good padding for aesthetics
  jwidget_set_padding(tab1, 15, 15, 15, 15);

  jlabel_create("Welcome to JustUI ClassPad!", tab1);
  jlabel_create("This is Tab 1.", tab1);
  jlabel_create("Select bottom buttons to navigate.", tab1);
  jlabel_create("The UI adjusts for you.", tab1);

  // ------------- TAB 2: Interactive -------------
  jwidget *tab2 = jwidget_create(stack);
  jlayout_set_vbox(tab2)->spacing = 15;
  jwidget_set_padding(tab2, 15, 15, 15, 15);

  jlabel_create("Features (Tab 2)", tab2);

  // A button to tap!
  jbutton *btn_action = jbutton_create("Click Me!", tab2);
  // Large padding for touchscreen friendliness
  jwidget_set_padding(btn_action, 15, 20, 15, 20);

  jlabel *t2_status = jlabel_create("Button not clicked yet.", tab2);

  // ------------- TAB 3: About -------------
  jwidget *tab3 = jwidget_create(stack);
  jlayout_set_vbox(tab3)->spacing = 10;
  jwidget_set_padding(tab3, 15, 15, 15, 15);

  jlabel_create("About JustUI (Tab 3)", tab3);
  jlabel_create("JustUI elegantly handles layout.", tab3);
  jlabel_create("It automatically supports touch!", tab3);
  jlabel_create("Stack layouts make tabs easy.", tab3);

  // ------------- TAB 4: USB -------------
  jwidget *tab4 = jwidget_create(stack);
  jlayout_set_vbox(tab4)->spacing = 2;
  jwidget_set_padding(tab4, 5, 5, 5, 5);

  jlabel *usb_status = jlabel_create("USB Status: Idle", tab4);

  jwidget *usb_cols = jwidget_create(tab4);
  jlayout_set_hbox(usb_cols)->spacing = 10; // Increased spacing for width padding
  jwidget_set_stretch(usb_cols, 1, 1, false);

  jwidget *col1 = jwidget_create(usb_cols);
  jlayout_set_vbox(col1)->spacing = 5; // Increased spacing for height
  jwidget_set_stretch(col1, 1, 1, false);

  jwidget *col2 = jwidget_create(usb_cols);
  jlayout_set_vbox(col2)->spacing = 5; // Increased spacing for height
  jwidget_set_stretch(col2, 1, 1, false);

  // Helper to create taller buttons (1.5x height approx by padding)
  // Standard padding was implicit or small. Let's make it larger.
  void style_usb_btn(jbutton *b) {
      // Top/Bottom 12 -> ~1.5x usual height feel?
      // Left/Right 5 -> just fit
      jwidget_set_padding(b, 12, 5, 12, 5);
  }

  // Column 1
  jbutton *btn_u1 = jbutton_create("Open", col1); style_usb_btn(btn_u1);
  jbutton *btn_u2 = jbutton_create("Open Wait", col1); style_usb_btn(btn_u2);
  jbutton *btn_u3 = jbutton_create("Close", col1); style_usb_btn(btn_u3);
  jbutton *btn_u4 = jbutton_create("WA TxtHead", col1); style_usb_btn(btn_u4);
  jbutton *btn_u5 = jbutton_create("WA ImgHead", col1); style_usb_btn(btn_u5);
  jbutton *btn_u6 = jbutton_create("WA Text", col1); style_usb_btn(btn_u6);
  jbutton *btn_u7 = jbutton_create("WA VRAM", col1); style_usb_btn(btn_u7);
  jbutton *btn_u14 = jbutton_create("Read Async", col1); style_usb_btn(btn_u14);

  // Column 2
  jbutton *btn_u8 = jbutton_create("WS TxtHead", col2); style_usb_btn(btn_u8);
  jbutton *btn_u9 = jbutton_create("WS ImgHead", col2); style_usb_btn(btn_u9);
  jbutton *btn_u10 = jbutton_create("WS Text", col2); style_usb_btn(btn_u10);
  jbutton *btn_u11 = jbutton_create("WS VRAM", col2); style_usb_btn(btn_u11);
  jbutton *btn_u12 = jbutton_create("Commit A", col2); style_usb_btn(btn_u12);
  jbutton *btn_u13 = jbutton_create("Commit S", col2); style_usb_btn(btn_u13);
  jbutton *btn_u15 = jbutton_create("Read Sync", col2); style_usb_btn(btn_u15);

  // ==========================================
  // BOTTOM NAVIGATION BAR
  // ==========================================
  jwidget *buttons = jwidget_create(scene);
  jlayout_set_hbox(buttons)->spacing = 6;
  // Don't stretch vertically, stretch horizontally
  jwidget_set_stretch(buttons, 1, 0, false);
  // Touch friendly paddings around the buttons bar
  jwidget_set_padding(buttons, 8, 8, 8, 8);

  jbutton *b_tab1 = jbutton_create("Tab 1", buttons);
  jwidget_set_padding(b_tab1, 8, 8, 8, 8);

  jbutton *b_tab2 = jbutton_create("Tab 2", buttons);
  jwidget_set_padding(b_tab2, 8, 8, 8, 8);

  jbutton *b_tab3 = jbutton_create("Tab 3", buttons);
  jwidget_set_padding(b_tab3, 8, 8, 8, 8);

  jbutton *b_tab4 = jbutton_create("USB", buttons);
  jwidget_set_padding(b_tab4, 8, 8, 8, 8);

  // Spacer to push the exit button to the right side of the screen
  jwidget *spacer = jwidget_create(buttons);
  jwidget_set_stretch(spacer, 1, 0, false);

  // Crucial: Exit button for touchscreen (ClassPad has no KEY_EXIT)
  jbutton *b_exit = jbutton_create("Exit", buttons);
  jwidget_set_padding(b_exit, 8, 12, 8, 12);

  // ==========================================
  // INITIALIZATION
  // ==========================================
  // Show tab1 by default and disable its button to show it's visually selected
  jscene_show_and_focus(scene, tab1);
  jbutton_set_disabled(b_tab1, true);
  jbutton_set_disabled(b_tab2, false);
  jbutton_set_disabled(b_tab3, false);
  jbutton_set_disabled(b_tab4, false);

  int action_count = 0;
  bool running = true;
  static char status_buf[64];

  // ==========================================
  // EVENT LOOP
  // ==========================================
  while (running)
  {
    jevent e = jscene_run(scene);

    if (e.type == JSCENE_PAINT)
    {
      dclear(C_WHITE);
      jscene_render(scene);
      dupdate();
    }
    else if (e.type == JBUTTON_TRIGGERED)
    {
      if (e.source == b_exit)
      {
        running = false;
      }
      else if (e.source == b_tab1)
      {
        jscene_show_and_focus(scene, tab1);
        jbutton_set_disabled(b_tab1, true);
        jbutton_set_disabled(b_tab2, false);
        jbutton_set_disabled(b_tab3, false);
        jbutton_set_disabled(b_tab4, false);
      }
      else if (e.source == b_tab2)
      {
        jscene_show_and_focus(scene, tab2);
        jbutton_set_disabled(b_tab1, false);
        jbutton_set_disabled(b_tab2, true);
        jbutton_set_disabled(b_tab3, false);
        jbutton_set_disabled(b_tab4, false);
      }
      else if (e.source == b_tab3)
      {
        jscene_show_and_focus(scene, tab3);
        jbutton_set_disabled(b_tab1, false);
        jbutton_set_disabled(b_tab2, false);
        jbutton_set_disabled(b_tab3, true);
        jbutton_set_disabled(b_tab4, false);
      }
      else if (e.source == b_tab4)
      {
        jscene_show_and_focus(scene, tab4);
        jbutton_set_disabled(b_tab1, false);
        jbutton_set_disabled(b_tab2, false);
        jbutton_set_disabled(b_tab3, false);
        jbutton_set_disabled(b_tab4, true);
      }
      else if (e.source == btn_action)
      {
        action_count++;
        // Use static buffer because jlabel_set_text doesn't copy the string
        // memory
        static char buf[64];
        sprintf(buf, "Clicked %d times!", action_count);
        jlabel_set_text(t2_status, buf);
      }
      // USB Handlers
      else if (e.source == btn_u1) { // Open
          int rc = usb_open(interfaces, GINT_CALL_NULL);
          sprintf(status_buf, "usb_open: %d", rc);
          jlabel_set_text(usb_status, status_buf);
      }
      else if (e.source == btn_u2) { // Open Wait
          usb_open_wait();
          jlabel_set_text(usb_status, "usb_open_wait: done");
      }
      else if (e.source == btn_u3) { // Close
          usb_close();
          jlabel_set_text(usb_status, "usb_close: done");
      }
      else if (e.source == btn_u4) { // WA TxtHead
          int rc = usb_write_async(usb_ff_bulk_output(), &header_text, sizeof(header_text), false, GINT_CALL_NULL);
          sprintf(status_buf, "WA TxtHead: %d", rc);
          jlabel_set_text(usb_status, status_buf);
      }
      else if (e.source == btn_u5) { // WA ImgHead
          int rc = usb_write_async(usb_ff_bulk_output(), &header_image, sizeof(header_image), false, GINT_CALL_NULL);
          sprintf(status_buf, "WA ImgHead: %d", rc);
          jlabel_set_text(usb_status, status_buf);
      }
      else if (e.source == btn_u6) { // WA Text
          int rc = usb_write_async(usb_ff_bulk_output(), short_text, strlen(short_text), false, GINT_CALL_NULL);
          sprintf(status_buf, "WA Text: %d", rc);
          jlabel_set_text(usb_status, status_buf);
      }
      else if (e.source == btn_u7) { // WA VRAM
          int rc = usb_write_async(usb_ff_bulk_output(), gint_vram, vram_size, false, GINT_CALL_NULL);
          sprintf(status_buf, "WA VRAM: %d", rc);
          jlabel_set_text(usb_status, status_buf);
      }
      else if (e.source == btn_u8) { // WS TxtHead
          int rc = usb_write_sync(usb_ff_bulk_output(), &header_text, sizeof(header_text), false);
          sprintf(status_buf, "WS TxtHead: %d", rc);
          jlabel_set_text(usb_status, status_buf);
      }
      else if (e.source == btn_u9) { // WS ImgHead
          int rc = usb_write_sync(usb_ff_bulk_output(), &header_image, sizeof(header_image), false);
          sprintf(status_buf, "WS ImgHead: %d", rc);
          jlabel_set_text(usb_status, status_buf);
      }
      else if (e.source == btn_u10) { // WS Text
          int rc = usb_write_sync(usb_ff_bulk_output(), short_text, strlen(short_text), false);
          sprintf(status_buf, "WS Text: %d", rc);
          jlabel_set_text(usb_status, status_buf);
      }
      else if (e.source == btn_u11) { // WS VRAM
          int rc = usb_write_sync(usb_ff_bulk_output(), gint_vram, vram_size, false);
          sprintf(status_buf, "WS VRAM: %d", rc);
          jlabel_set_text(usb_status, status_buf);
      }
      else if (e.source == btn_u12) { // Commit A
          int rc = usb_commit_async(usb_ff_bulk_output(), GINT_CALL_NULL);
          sprintf(status_buf, "Commit A: %d", rc);
          jlabel_set_text(usb_status, status_buf);
      }
      else if (e.source == btn_u13) { // Commit S
          usb_commit_sync(usb_ff_bulk_output());
          jlabel_set_text(usb_status, "Commit S: done");
      }
      else if (e.source == btn_u14) { // Read Async
          // Read up to 64 bytes
          int rc = usb_read_async(usb_ff_bulk_input(), read_buffer, 64, USB_READ_IGNORE_ZEROS, NULL, NULL, GINT_CALL_NULL);
          sprintf(status_buf, "Read A started: %d", rc);
          jlabel_set_text(usb_status, status_buf);
      }
      else if (e.source == btn_u15) { // Read Sync
          // Read up to 64 bytes (blocking)
          memset(read_buffer, 0, 64);
          int rc = usb_read_sync(usb_ff_bulk_input(), read_buffer, 64, false);
          if (rc > 0) {
              // Ensure null termination for display if it's text
              read_buffer[rc < 64 ? rc : 63] = 0;
              // Try to display first few chars
              char disp[32];
              strncpy(disp, read_buffer, 30);
              disp[30] = 0;
              sprintf(status_buf, "Read %d: %s", rc, disp);
          } else {
              sprintf(status_buf, "Read Sync err: %d", rc);
          }
          jlabel_set_text(usb_status, status_buf);
      }
    }
    else if (e.type == JWIDGET_KEY && e.key.type == KEYEV_DOWN)
    {
      // Hardware fallback for exiting (if available)
      if (e.key.key == KEY_CLEAR || e.key.key == KEY_EXIT ||
          e.key.key == KEY_HOME)
      {
        running = false;
      }
    }
  }

  // Usually scenes clean up all their children
  jwidget_destroy(scene);

  return 1;
}
