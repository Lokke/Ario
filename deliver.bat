@echo off

rmdir /s /q delivery

mkdir delivery

copy ario.exe delivery
copy ario.dll delivery

mkdir delivery\data
copy data\*.png delivery\data
copy data\profiles.xml.default delivery\data
copy data\ario-ui.xml delivery\data\

mkdir delivery\data\glade
copy data\glade\*.glade delivery\data\glade\

mkdir delivery\plugins
copy plugins\filesystem\libfilesystem.dll delivery\plugins
copy plugins\filesystem\filesystem-ui.xml delivery\plugins
copy plugins\filesystem\filesystem.ario-plugin.w32 delivery\plugins\filesystem.ario-plugin

copy plugins\wikipedia\libwikipedia.dll delivery\plugins
copy plugins\wikipedia\wikipedia-ui.xml delivery\plugins
copy plugins\wikipedia\wikipedia.png delivery\plugins
copy plugins\wikipedia\wikipedia.ario-plugin.w32 delivery\plugins\wikipedia.ario-plugin

copy plugins\radios\libradios.dll delivery\plugins
copy plugins\radios\radios-ui.xml delivery\plugins
copy plugins\radios\radios.xml.default delivery\plugins
copy plugins\radios\radios.ario-plugin.w32 delivery\plugins\radios.ario-plugin
