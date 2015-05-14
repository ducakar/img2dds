( cd linux32 && make install DESTDIR=. )
( cd linux64 && make install DESTDIR=. )
( cd win32 && make install DESTDIR=. )

mv -f linux32/usr/local/bin/img2dds img2dds/linux32
mv -f linux64/usr/local/bin/img2dds img2dds/linux64
mv -f win32/usr/local/bin/img2dds.exe img2dds/win32

strip img2dds/linux32/*
strip img2dds/linux64/*
i686-w64-mingw32-strip img2dds/win32/*

# local copy
cp linux64/img2dds ~/.local/bin
strip ~/.local/bin/img2dds
