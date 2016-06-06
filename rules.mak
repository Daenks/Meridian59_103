# common rules
#
# to be included at the end of each makefile

makedirs:
        -@mkdir $(OUTDIR) >nul 2>&1

{$(SOURCEDIR)}.c{$(OUTDIR)}.obj::
	$(CC) $(CFLAGS) /FpCpch /Fd$(OUTDIR)\ /Fo$(OUTDIR)/ -c $< 

{$(SOURCEDIR)}.cpp{$(OUTDIR)}.obj::
	$(CC) $(CFLAGS) /FpCpch /Fd$(OUTDIR)\ /Fo$(OUTDIR)/ -c $< 

{$(OUTDIR)}.c{$(OUTDIR)}.obj::
	$(CC) $(CFLAGS) /Fd$(OUTDIR)\ /Fo$(OUTDIR)/ -I$(SOURCEDIR) -c $< 

{$(SOURCEDIR)}.rc{$(OUTDIR)}.res:
        $(RC) /fo $@ /i$(SOURCEDIR) $**

clean :
        @$(RM) $(OUTDIR)\* >nul 2>&1
        @$(RMDIR) $(OUTDIR) > nul 2>&1
