@rem windows compile of swiss file knife
cl sfk.cpp patch.cpp inst.cpp kernel32.lib user32.lib gdi32.lib ws2_32.lib -DSFK_WINPOPUP_SUPPORT

@rem compile of sfk snapview (for windows only)
cl sview.cpp sfk.cpp -D USE_SFK_BASE kernel32.lib user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS
