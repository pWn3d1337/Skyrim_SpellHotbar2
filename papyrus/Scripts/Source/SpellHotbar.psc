scriptName SpellHotbar hidden

int function getNumberOfSlots() global native
int function setNumberOfSlots(int num) global native
float function getSlotScale() global native
float function setSlotScale(float scale) global native
float function getOffsetX(bool rescale) global native
float function setOffsetX(float value, bool rescale) global native
float function getOffsetY(bool rescale) global native
float function setOffsetY(float value, bool rescale) global native
int function getInheritMode(int id) global native
int function setInheritMode(int id, int mode) global native
bool function getBarEnabled(int id) native global
bool function toggleBarEnabled(int id) native global
int function setHudBarShowMode(int mode) native global
int function getHudBarShowMode() native global
function reloadResources() native global
function reloadData() native global
function showDragBar() native global
int function setHudBarShowModeVampireLord(int mode) native global
int function getHudBarShowModeVampireLord() native global
int function setHudBarShowModeWerewolf(int mode) native global
int function getHudBarShowModeWerewolf() native global
bool function saveBarsToFile(String filenname) native global
bool function fileExists(String filenname) native global
bool function loadBarsFromFile(String filename_mod_dir, String filename_user_dir) native global
function clearBars() native global
float function getSlotSpacing() global native
float function setSlotSpacing(float spacing) global native
int function getTextShowMode() native global
int function setTextShowMode(int mode) native global
bool function isTransformedFavMenuBind() native global
bool function isDefaultBarWhenSheathed() global native
bool function toggleDefaultBarWhenSheathed() global native
bool function isDisableMenuRendering() global native
bool function toggleDisableMenuRendering() global native
bool function castSpell(Spell spell) global native
int function setKeyBind(int slot, int keyCode) global native
int function getKeyBind(int slot) global native
bool function isNonModBarDisabled() global native
bool function toggleDisableNonModBar() global native
function openSpellEditor() native global
