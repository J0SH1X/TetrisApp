#include "Core.h"

void timerCallback( EFI_EVENT event, void* context );
void togglePause( Core* this );
void gameOver( Core* this );
void startGame( Core* this );
void resetTickCounter( Core* this );

void handleInput( Core* this ) {
	EFI_INPUT_KEY key;
	EFI_STATUS status;

	status = gST->ConIn->ReadKeyStroke( gST->ConIn, &key );

	if ( status != EFI_NOT_READY ) {
		ASSERT_EFI_ERROR( status );

		if ( this->gameState == GAME_STATE_WAITING_FOR_START ) {
			startGame( this );
		}
		else if ( key.UnicodeChar == L'p' || key.UnicodeChar == L'P' ) {
			if ( this->gameState == GAME_STATE_RUNNING || this->gameState == GAME_STATE_PAUSED ) {
				togglePause( this );
			}
		}
		else if ( key.UnicodeChar == L' ' ) {
			if ( this->gameState == GAME_STATE_RUNNING ) {
				this->board->dropPiece( this->board );
				resetTickCounter( this );
			}
		}
		else {
if (key.UnicodeChar == L'\r' || key.UnicodeChar == L'\n'){
   	if ( this->gameState == GAME_STATE_RUNNING ) {
		this->board->rotatePiece( this->board );
		}
}else if(key.ScanCode == SCAN_RIGHT){
    if ( this->gameState == GAME_STATE_RUNNING ) {
		if ( !this->board->movePieceDown( this->board ) ) {
			resetTickCounter( this );
		}
		}
}else if (key.ScanCode == SCAN_UP){
  	if ( this->gameState == GAME_STATE_RUNNING ) {
		this->board->movePieceLeft( this->board );
		}  
}else if(key.ScanCode == SCAN_DOWN){
    if ( this->gameState == GAME_STATE_RUNNING ) {
		this->board->movePieceRight( this->board );
	}
}else if (key.ScanCode == SCAN_UP && SCAN_DOWN){
    this->gameState = GAME_STATE_EXIT;
}else if(key.ScanCode == SCAN_PAGE_UP){
    if ( this->gameState != GAME_STATE_OVER && this->board->level < MAX_LEVEL ) {
		this->board->level++;
	}
}else if(key.ScanCode == SCAN_PAGE_DOWN){
    	if ( this->gameState != GAME_STATE_OVER && this->board->level > 1 ) {
			this->board->level--;
			}
}
		}
	}
}



void drawWindow( Core* this ) {
	if ( this->gameState == GAME_STATE_RUNNING && this->board->blocked ) {
		gameOver( this );
	}

	if ( this->tickCounter == 0 ) {
		this->board->movePieceDown( this->board );

		resetTickCounter( this );
	}

	this->board->drawBoard( this->board );
}



void ConstructCore( Core** this ) {
	EFI_STATUS status;
	Core* core = AllocatePool( sizeof( Core ) );
	ZeroMem( core, sizeof( Core ) );

	ConstructBoard( &core->board );

	core->gameState = GAME_STATE_WAITING_FOR_START;
	core->handleInput = handleInput;
	core->drawWindow = drawWindow;
	core->tickCounter = GAME_REFRESH_PERIOD;

	// set up timer event
	status = gBS->CreateEventEx( EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, timerCallback, core, NULL, &core->timerEvent );
	ASSERT_EFI_ERROR( status );

	// start the timer 
	status = gBS->SetTimer( core->timerEvent, TimerPeriodic, TIMER_PERIOD );
	ASSERT_EFI_ERROR( status );

	gST->ConOut->SetAttribute( gST->ConOut, EFI_TEXT_ATTR( EFI_WHITE, EFI_RED ) );
	setCursorPos( BOARD_TOP_X, 1 );
	Print( L" PRESS ANY KEY TO START " );

	*this = core;
}



void DestructCore( Core* this ) {
	EFI_STATUS status = gBS->CloseEvent( this->timerEvent );
	ASSERT_EFI_ERROR( status );
	DestructBoard( this->board );
	FreePool( this );
}



void timerCallback( EFI_EVENT event, void* context ) {
	Core* core = context;

	if ( core->gameState == GAME_STATE_RUNNING && core->tickCounter != 0 )
		core->tickCounter--;
}



void togglePause( Core* this ) {
	this->gameState = this->gameState == GAME_STATE_PAUSED ? GAME_STATE_RUNNING : GAME_STATE_PAUSED;


	setCursorPos( BOARD_TOP_X + BOARD_WIDTH - 6, 1 );
	if ( this->gameState == GAME_STATE_PAUSED ) {
		gST->ConOut->SetAttribute( gST->ConOut, EFI_TEXT_ATTR( EFI_WHITE, EFI_GREEN ) );
		Print( L" GAME PAUSED " );
	}
	else {
		gST->ConOut->SetAttribute( gST->ConOut, EFI_TEXT_ATTR( EFI_BLACK, EFI_BLACK ) );
		Print( L"             " );
	}
}



void gameOver( Core* this ) {
	this->gameState = GAME_STATE_OVER;
	gST->ConOut->SetAttribute( gST->ConOut, EFI_TEXT_ATTR( EFI_WHITE, EFI_RED ) );
	setCursorPos( BOARD_TOP_X + BOARD_WIDTH - 6, 1 );
	Print( L" GAME OVER " );
}



void startGame( Core* this ) {
	gST->ConOut->SetAttribute( gST->ConOut, EFI_TEXT_ATTR( EFI_BLACK, EFI_BLACK ) );
	setCursorPos( BOARD_TOP_X, 1 );
	Print( L"                        " );

	this->gameState = GAME_STATE_RUNNING;
}



void resetTickCounter( Core* this ) {
	//    lvl   1    2   ...  10  
	// period 100%  90%  ...  10%
	this->tickCounter = ( GAME_REFRESH_PERIOD * ( 11 - this->board->level ) ) / 10;
}