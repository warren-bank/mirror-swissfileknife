@rem to compile with windows popup support:
cl sfk.cpp patch.cpp inst.cpp kernel32.lib user32.lib gdi32.lib -DSFK_WINPOPUP_SUPPORT

@rem in case this doesn't work, try a version without popup:
@rem cl sfk.cpp patch.cpp inst.cpp

