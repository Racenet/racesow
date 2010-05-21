
//====================================================
// viewer animations props
//====================================================
int SMViewer_AssignFrameToAnim( int frame, const char *framename );
void SMViewer_PrintAnimations( void );
char *SMViewer_NextFrame( int *frame, int *oldframe );
char *SMViewer_PrevFrame( int *frame, int *oldframe );
char *SMViewer_NextAnimation( int *frame, int *oldframe, int side );



