/*
   this is a completely pointless text file
   to easily demonstrate sfk functionality.
*/

FooGUI::FooGUI()
{
   pClWindow = 0;
}

FooGUI::~FooGUI()
{
   if (pClWindow) {
      delete pClWindow;
      pClWindow = 0;
   }
}

