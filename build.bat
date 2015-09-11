CMD /k "V:\Microsoft Visual Studio 12.0\Common7\Tools\vsvars32.bat"

mkdir .\run\server\loadkod
mkdir .\run\server\rsc
mkdir .\run\server\channel
mkdir .\run\server\memmap
mkdir .\run\server\savegame
copy .\run\localclient\*.dll .\run\server\
copy .\run\localclient\wrap.dll .\bin\