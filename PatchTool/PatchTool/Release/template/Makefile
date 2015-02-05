NAME = DiyPatch
OBJS = $(NAME).obj
RES  = $(NAME).res

LINK_FLAG = /subsystem:windows
ML_FLAG = /c /coff

$(NAME).exe: $(OBJS) $(RES)
	Link $(LINK_FLAG) $(OBJS) $(RES)

$(NAME).obj:addcode  Patch
	ml $(ML_FLAG) $(NAME)
#.rc.res:
#	rc $<

clean:
	del *.obj
#	del *.res
