bmconv c64icon_er6.lst
call aiftool e32frodo e32frodo.mbm
xcopy e32frodo.aif \epoc32\release\wins\udeb\z\system\apps\e32frodo\*.*
xcopy e32frodo.aif \epoc32\release\wins\urel\z\system\apps\e32frodo\*.*
xcopy e32frodo.aif \epoc32\release\armi\udeb\*.*
xcopy e32frodo.aif \epoc32\release\armi\urel\*.*
