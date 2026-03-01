#include "flow_boot_wizard.h"

#include "ui_actions.h"

UIState g_bootWizardAfterOkState = UIState::BOOT;
Tab     g_bootWizardAfterOkTab   = Tab::TAB_PET;

void bootWizardBegin(UIState afterOkState, Tab afterOkTab)
{
  g_bootWizardAfterOkState = afterOkState;
  g_bootWizardAfterOkTab   = afterOkTab;

  // Start at the WiFi prompt step.
  uiActionEnterState(UIState::BOOT_WIFI_PROMPT, Tab::TAB_PET, true);
}
