extern struct bluetooth_plugin_desc __bluetooth_builtin_hostname;
extern struct bluetooth_plugin_desc __bluetooth_builtin_wiimote;
extern struct bluetooth_plugin_desc __bluetooth_builtin_autopair;
extern struct bluetooth_plugin_desc __bluetooth_builtin_policy;
extern struct bluetooth_plugin_desc __bluetooth_builtin_a2dp;
extern struct bluetooth_plugin_desc __bluetooth_builtin_avrcp;
extern struct bluetooth_plugin_desc __bluetooth_builtin_network;
extern struct bluetooth_plugin_desc __bluetooth_builtin_input;
extern struct bluetooth_plugin_desc __bluetooth_builtin_hog;
extern struct bluetooth_plugin_desc __bluetooth_builtin_gap;
extern struct bluetooth_plugin_desc __bluetooth_builtin_scanparam;
extern struct bluetooth_plugin_desc __bluetooth_builtin_deviceinfo;
extern struct bluetooth_plugin_desc __bluetooth_builtin_battery;
extern struct bluetooth_plugin_desc __bluetooth_builtin_bap;
extern struct bluetooth_plugin_desc __bluetooth_builtin_mcp;
extern struct bluetooth_plugin_desc __bluetooth_builtin_vcp;

static struct bluetooth_plugin_desc *__bluetooth_builtin[] = {
  &__bluetooth_builtin_hostname,
  &__bluetooth_builtin_wiimote,
  &__bluetooth_builtin_autopair,
  &__bluetooth_builtin_policy,
  &__bluetooth_builtin_a2dp,
  &__bluetooth_builtin_avrcp,
  &__bluetooth_builtin_network,
  &__bluetooth_builtin_input,
  &__bluetooth_builtin_hog,
  &__bluetooth_builtin_gap,
  &__bluetooth_builtin_scanparam,
  &__bluetooth_builtin_deviceinfo,
  &__bluetooth_builtin_battery,
  &__bluetooth_builtin_bap,
  &__bluetooth_builtin_mcp,
  &__bluetooth_builtin_vcp,
  NULL
};
