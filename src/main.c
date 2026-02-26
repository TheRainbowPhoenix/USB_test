#include <gint/display.h>
#include <gint/keyboard.h>
#include <justui/jbutton.h>
#include <justui/jlabel.h>
#include <justui/jlayout.h>
#include <justui/jscene.h>
#include <justui/jwidget.h>
#include <stdio.h>

int main(void)
{
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
  jwidget_set_padding(b_tab1, 12, 12, 12, 12);

  jbutton *b_tab2 = jbutton_create("Tab 2", buttons);
  jwidget_set_padding(b_tab2, 12, 12, 12, 12);

  jbutton *b_tab3 = jbutton_create("Tab 3", buttons);
  jwidget_set_padding(b_tab3, 12, 12, 12, 12);

  // Spacer to push the exit button to the right side of the screen
  jwidget *spacer = jwidget_create(buttons);
  jwidget_set_stretch(spacer, 1, 0, false);

  // Crucial: Exit button for touchscreen (ClassPad has no KEY_EXIT)
  jbutton *b_exit = jbutton_create("Exit", buttons);
  jwidget_set_padding(b_exit, 12, 16, 12, 16);

  // ==========================================
  // INITIALIZATION
  // ==========================================
  // Show tab1 by default and disable its button to show it's visually selected
  jscene_show_and_focus(scene, tab1);
  jbutton_set_disabled(b_tab1, true);
  jbutton_set_disabled(b_tab2, false);
  jbutton_set_disabled(b_tab3, false);

  int action_count = 0;
  bool running = true;

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
      }
      else if (e.source == b_tab2)
      {
        jscene_show_and_focus(scene, tab2);
        jbutton_set_disabled(b_tab1, false);
        jbutton_set_disabled(b_tab2, true);
        jbutton_set_disabled(b_tab3, false);
      }
      else if (e.source == b_tab3)
      {
        jscene_show_and_focus(scene, tab3);
        jbutton_set_disabled(b_tab1, false);
        jbutton_set_disabled(b_tab2, false);
        jbutton_set_disabled(b_tab3, true);
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
