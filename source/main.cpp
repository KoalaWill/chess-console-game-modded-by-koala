#include "includes.h"
#include "user_interface.h"
#include "chess.h"
#include "serialib.h"
#include "debug.h"
#include <windows.h>

//---------------------------------------------------------------------------------------
// Global variable
//---------------------------------------------------------------------------------------
Game *current_game = NULL;
bool arduino_mode = false;
bool AI_mode = false;
string port_connected;
// TODO mega setting
#define baud 9600
// stings saved for communicating with StockFish
string command = "position startpos move ";
// TODO string of connected port

// TODO arduino serial scan
string Scan_COMS()
{
   serialib device;
   cout << "Scaning your device...\n";
   string cur_COM = "COMX";
   bool found = false;
   string found_port;
   for (int i = 1; i <= 255; i++)
   {
      if (i <= 10)
      {
         cur_COM.pop_back();
      }
      else if (i <= 100)
      {
         cur_COM.pop_back();
         cur_COM.pop_back();
      }
      else
      {
         cur_COM.pop_back();
         cur_COM.pop_back();
         cur_COM.pop_back();
      }
      cur_COM += to_string(i);
      // cout << "Now trying --> " << cur_COM << endl;
      // try to connect to the arduino
      if (device.openDevice(cur_COM.c_str(), baud) == 1)
      {
         cout << "Device detected on " << cur_COM << endl;
         cout << "Successfully connected to your device !\n";
         found = true;
         found_port = cur_COM;
         // Close the device before testing the next port
         device.closeDevice();
         break;
      }
   }
   if (found == false)
   {
      cout << "I can't find your device.\n";
      return 0;
   }
   else
   {
      port_connected = found_port;
      return found_port;
   }
}
// TODO arduino serial send
void Send_string(string port, string message)
{
   serialib device;
   // Connection to serial port
   char errorOpening = device.openDevice(port.c_str(), baud);
   // If connection fails, return the error code otherwise, display a success message
   if (errorOpening != 1){
      cout << errorOpening;
      return;
   }
   // Create the buffer string
   char buffer[200];

   const int length = message.length();

   // declaring character array (+1 for null terminator)
   char *temp_char_array = new char[length + 1];

   // copying the contents of the string to char array
   strcpy(temp_char_array, message.c_str());

   // getting the command into to the buffer
   for (int i = 0; i < length; i++)
   {
      buffer[i] = temp_char_array[i];
   }
   delete[] temp_char_array;

   // Write the string on the serial device
   device.writeString(buffer);
   //  cout << "String sent: " << buffer;

   // Close the serial device
   device.closeDevice();
}
// TODO arduino serial read line
string Read_line(string port)
{
   serialib device;
   // Connection to serial port
   char errorOpening = device.openDevice(port.c_str(), baud);
   // If connection fails, return the error code otherwise, display a success message
   if (errorOpening != 1)
      cout << errorOpening;
   // Create the buffer string
   char buffer[200];
   // read the string till \n for the device
   device.readString(buffer, '\n', 100, 2000);
   return buffer;
}


//---------------------------------------------------------------------------------------
// Helper
// Auxiliar functions to determine if a move is valid, etc
//---------------------------------------------------------------------------------------
bool isMoveValid(Chess::Position present, Chess::Position future, Chess::EnPassant *S_enPassant, Chess::Castling *S_castling, Chess::Promotion *S_promotion)
{
   bool bValid = false;

   char chPiece = current_game->getPieceAtPosition(present.iRow, present.iColumn);

   // ----------------------------------------------------
   // 1. Is the piece  allowed to move in that direction?
   // ----------------------------------------------------
   switch (toupper(chPiece))
   {
   case 'P':
   {
      // Wants to move forward
      if (future.iColumn == present.iColumn)
      {
         // Simple move forward
         if ((Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 1) ||
             (Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 1))
         {
            if (EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow, future.iColumn))
            {
               bValid = true;
            }
         }

         // Double move forward
         else if ((Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 2) ||
                  (Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 2))
         {
            // This is only allowed if the pawn is in its original place
            if (Chess::isWhitePiece(chPiece))
            {
               if (EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow - 1, future.iColumn) &&
                   EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow, future.iColumn) &&
                   1 == present.iRow)
               {
                  bValid = true;
               }
            }
            else // if ( isBlackPiece(chPiece) )
            {
               if (EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow + 1, future.iColumn) &&
                   EMPTY_SQUARE == current_game->getPieceAtPosition(future.iRow, future.iColumn) &&
                   6 == present.iRow)
               {
                  bValid = true;
               }
            }
         }
         else
         {
            // This is invalid
            return false;
         }
      }

      // The "en passant" move
      else if ((Chess::isWhitePiece(chPiece) && 4 == present.iRow && 5 == future.iRow && 1 == abs(future.iColumn - present.iColumn)) ||
               (Chess::isBlackPiece(chPiece) && 3 == present.iRow && 2 == future.iRow && 1 == abs(future.iColumn - present.iColumn)))
      {
         // It is only valid if last move of the opponent was a double move forward by a pawn on a adjacent column
         string last_move = current_game->getLastMove();

         // Parse the line
         Chess::Position LastMoveFrom;
         Chess::Position LastMoveTo;
         current_game->parseMove(last_move, &LastMoveFrom, &LastMoveTo);

         // First of all, was it a pawn?
         char chLstMvPiece = current_game->getPieceAtPosition(LastMoveTo.iRow, LastMoveTo.iColumn);

         if (toupper(chLstMvPiece) != 'P')
         {
            return false;
         }

         // Did the pawn have a double move forward and was it an adjacent column?
         if (2 == abs(LastMoveTo.iRow - LastMoveFrom.iRow) && 1 == abs(LastMoveFrom.iColumn - present.iColumn))
         {
            cout << "En passant move!\n";
            if(arduino_mode) Send_string(port_connected, "En passant move!\n");
            bValid = true;

            S_enPassant->bApplied = true;
            S_enPassant->PawnCaptured.iRow = LastMoveTo.iRow;
            S_enPassant->PawnCaptured.iColumn = LastMoveTo.iColumn;
         }
      }

      // Wants to capture a piece
      else if (1 == abs(future.iColumn - present.iColumn))
      {
         if ((Chess::isWhitePiece(chPiece) && future.iRow == present.iRow + 1) || (Chess::isBlackPiece(chPiece) && future.iRow == present.iRow - 1))
         {
            // Only allowed if there is something to be captured in the square
            if (EMPTY_SQUARE != current_game->getPieceAtPosition(future.iRow, future.iColumn))
            {
               bValid = true;
               cout << "Pawn captured a piece!\n";
               if(arduino_mode) Send_string(port_connected, "Pawn captured a piece!\n");
            }
         }
      }
      else
      {
         // This is invalid
         return false;
      }

      // If a pawn reaches its eight rank, it must be promoted to another piece
      if ((Chess::isWhitePiece(chPiece) && 7 == future.iRow) ||
          (Chess::isBlackPiece(chPiece) && 0 == future.iRow))
      {
         cout << "Pawn must be promoted!\n";
         if(arduino_mode) Send_string(port_connected, "ERR Pawn must be promoted!\n");
         S_promotion->bApplied = true;
      }
   }
   break;

   case 'R':
   {
      // Horizontal move
      if ((future.iRow == present.iRow) && (future.iColumn != present.iColumn))
      {
         // Check if there are no pieces on the way
         if (current_game->isPathFree(present, future, Chess::HORIZONTAL))
         {
            bValid = true;
         }
         else if(arduino_mode)
         {
            Send_string(port_connected, "ERR Rook path not free\n");
         }
      }
      // Vertical move
      else if ((future.iRow != present.iRow) && (future.iColumn == present.iColumn))
      {
         // Check if there are no pieces on the way
         if (current_game->isPathFree(present, future, Chess::VERTICAL))
         {
            bValid = true;
         }
         else if(arduino_mode)
         {
            Send_string(port_connected, "ERR Rook path not free\n");
         }
      }
   }
   break;

   case 'N':
   {
      if ((2 == abs(future.iRow - present.iRow)) && (1 == abs(future.iColumn - present.iColumn)))
      {
         bValid = true;
      }

      else if ((1 == abs(future.iRow - present.iRow)) && (2 == abs(future.iColumn - present.iColumn)))
      {
         bValid = true;
      }
      else if(arduino_mode)
      {
         Send_string(port_connected, "ERR knight error\n");
      }
   }
   break;

   case 'B':
   {
      // Diagonal move
      if (abs(future.iRow - present.iRow) == abs(future.iColumn - present.iColumn))
      {
         // Check if there are no pieces on the way
         if (current_game->isPathFree(present, future, Chess::DIAGONAL))
         {
            bValid = true;
         }
         else if(arduino_mode)
         {
            Send_string(port_connected, "ERR Bishop path not free\n");
         }
      }
   }
   break;

   case 'Q':
   {
      // Horizontal move
      if ((future.iRow == present.iRow) && (future.iColumn != present.iColumn))
      {
         // Check if there are no pieces on the way
         if (current_game->isPathFree(present, future, Chess::HORIZONTAL))
         {
            bValid = true;
         }
         else if(arduino_mode)
         {
            Send_string(port_connected, "ERR Queen path not free\n");
         }
      }
      // Vertical move
      else if ((future.iRow != present.iRow) && (future.iColumn == present.iColumn))
      {
         // Check if there are no pieces on the way
         if (current_game->isPathFree(present, future, Chess::VERTICAL))
         {
            bValid = true;
         }
         else if(arduino_mode)
         {
            Send_string(port_connected, "ERR Queen path not free\n");
         }
      }

      // Diagonal move
      else if (abs(future.iRow - present.iRow) == abs(future.iColumn - present.iColumn))
      {
         // Check if there are no pieces on the way
         if (current_game->isPathFree(present, future, Chess::DIAGONAL))
         {
            bValid = true;
         }
         else if(arduino_mode)
         {
            Send_string(port_connected, "ERR Queen path not free\n");
         }
      }
   }
   break;

   case 'K':
   {
      // Horizontal move by 1
      if ((future.iRow == present.iRow) && (1 == abs(future.iColumn - present.iColumn)))
      {
         bValid = true;
      }

      // Vertical move by 1
      else if ((future.iColumn == present.iColumn) && (1 == abs(future.iRow - present.iRow)))
      {
         bValid = true;
      }

      // Diagonal move by 1
      else if ((1 == abs(future.iRow - present.iRow)) && (1 == abs(future.iColumn - present.iColumn)))
      {
         bValid = true;
      }

      // Castling
      else if ((future.iRow == present.iRow) && (2 == abs(future.iColumn - present.iColumn)))
      {
         // Castling is only allowed in these circunstances:

         // 1. King is not in check
         if (true == current_game->playerKingInCheck())
         {
            return false;
         }

         // 2. No pieces in between the king and the rook
         if (false == current_game->isPathFree(present, future, Chess::HORIZONTAL))
         {
            return false;
         }

         // 3. King and rook must not have moved yet;
         // 4. King must not pass through a square that is attacked by an enemy piece
         if (future.iColumn > present.iColumn)
         {
            // if future.iColumn is greather, it means king side
            if (false == current_game->castlingAllowed(Chess::Side::KING_SIDE, Chess::getPieceColor(chPiece)))
            {
               createNextMessage("Castling to the king side is not allowed.\n");
               if(arduino_mode) Send_string(port_connected, "ERR Castling to the king side is not allowed.\n");
               return false;
            }
            else
            {
               // Check if the square that the king skips is not under attack
               Chess::UnderAttack square_skipped = current_game->isUnderAttack(present.iRow, present.iColumn + 1, current_game->getCurrentTurn());
               if (false == square_skipped.bUnderAttack)
               {
                  // Fill the S_castling structure
                  S_castling->bApplied = true;

                  // Present position of the rook
                  S_castling->rook_before.iRow = present.iRow;
                  S_castling->rook_before.iColumn = present.iColumn + 3;

                  // Future position of the rook
                  S_castling->rook_after.iRow = future.iRow;
                  S_castling->rook_after.iColumn = present.iColumn + 1; // future.iColumn -1

                  bValid = true;
               }
            }
         }
         else // if (future.iColumn < present.iColumn)
         {
            // if present.iColumn is greather, it means queen side
            if (false == current_game->castlingAllowed(Chess::Side::QUEEN_SIDE, Chess::getPieceColor(chPiece)))
            {
               createNextMessage("Castling to the queen side is not allowed.\n");
               if(arduino_mode) Send_string(port_connected, "ERR Castling to the queen side is not allowed.\n");
               return false;
            }
            else
            {
               // Check if the square that the king skips is not attacked
               Chess::UnderAttack square_skipped = current_game->isUnderAttack(present.iRow, present.iColumn - 1, current_game->getCurrentTurn());
               if (false == square_skipped.bUnderAttack)
               {
                  // Fill the S_castling structure
                  S_castling->bApplied = true;

                  // Present position of the rook
                  S_castling->rook_before.iRow = present.iRow;
                  S_castling->rook_before.iColumn = present.iColumn - 4;

                  // Future position of the rook
                  S_castling->rook_after.iRow = future.iRow;
                  S_castling->rook_after.iColumn = present.iColumn - 1; // future.iColumn +1

                  bValid = true;
               }
            }
         }
      }
      else
      {
         Send_string(port_connected, "ERR King error move\n");
      }
   }
   break;

   default:
   {
      cout << "!!!!Should not reach here. Invalid piece: " << char(chPiece) << "\n\n\n";
      if(arduino_mode){
         string tempA = "ERR !!!!Should not reach here. Invalid piece: ";
         tempA += char(chPiece);
         tempA += '\n';
         Send_string(port_connected, tempA);
      }
   }
   break;
   }

   // If it is a move in an invalid direction, do not even bother to check the rest
   if (false == bValid)
   {
      cout << "Piece is not allowed to move to that square\n";
      if(arduino_mode) Send_string(port_connected, "ERR Piece is not allowed to move to that square\n");
      return false;
   }

   // -------------------------------------------------------------------------
   // 2. Is there another piece of the same color on the destination square?
   // -------------------------------------------------------------------------
   if (current_game->isSquareOccupied(future.iRow, future.iColumn))
   {
      char chAuxPiece = current_game->getPieceAtPosition(future.iRow, future.iColumn);
      if (Chess::getPieceColor(chPiece) == Chess::getPieceColor(chAuxPiece))
      {
         cout << "Position is already taken by a piece of the same color\n";
         if(arduino_mode) Send_string(port_connected, "ERR Position is already taken by a piece of the same color\n");
         return false;
      }
   }

   // ----------------------------------------------
   // 3. Would the king be in check after the move?
   // ----------------------------------------------
   if (true == current_game->wouldKingBeInCheck(chPiece, present, future, S_enPassant))
   {
      cout << "Move would put player's king in check\n";
      if(arduino_mode) Send_string(port_connected, "ERR Move would put player's king in check\n");
      return false;
   }

   return bValid;
}

void makeTheMove(Chess::Position present, Chess::Position future, Chess::EnPassant *S_enPassant, Chess::Castling *S_castling, Chess::Promotion *S_promotion)
{
   char chPiece = current_game->getPieceAtPosition(present.iRow, present.iColumn);

   // -----------------------
   // Captured a piece?
   // -----------------------
   if (current_game->isSquareOccupied(future.iRow, future.iColumn))
   {
      char chAuxPiece = current_game->getPieceAtPosition(future.iRow, future.iColumn);

      if (Chess::getPieceColor(chPiece) != Chess::getPieceColor(chAuxPiece))
      {
         createNextMessage(Chess::describePiece(chAuxPiece) + " captured!\n");
         if(arduino_mode){
            string tempA;
            tempA.clear();
            tempA = Chess::describePiece(chAuxPiece);
            tempA += " captured!\n";
            Send_string(port_connected, tempA);
         }
      }
      else
      {
         cout << "Error. We should not be making this move\n";
         if(arduino_mode) Send_string(port_connected, "Error. We should not be making this move\n");
         throw("ERR Error. We should not be making this move");
      }
   }
   else if (true == S_enPassant->bApplied)
   {
      createNextMessage("Pawn captured by \"en passant\" move!\n");
      if(arduino_mode) Send_string(port_connected, "Pawn captured by \"en passant\" move!\n");
   }

   if ((true == S_castling->bApplied))
   {
      createNextMessage("Castling applied!\n");
      if(arduino_mode) Send_string(port_connected, "Castling applied!\n");
   }

   current_game->movePiece(present, future, S_enPassant, S_castling, S_promotion);
}

//---------------------------------------------------------------------------------------
// Commands
// Functions to handle the commands of the program
// New game, Move, Undo, Save game, Load game, etc
//---------------------------------------------------------------------------------------
void newGame(void)
{
   if (NULL != current_game)
   {
      command = "position startpos move ";
      delete current_game;
   }

   current_game = new Game();
}
void undoMove(void)
{
   if(AI_mode){
      createNextMessage("Undo is not possible now!\n");
      if(arduino_mode) Send_string(port_connected, "ERR Undo is not possible now!\n");
   }
   else{
      if (false == current_game->undoIsPossible())
      {
         createNextMessage("Undo is not possible now!\n");
         if(arduino_mode) Send_string(port_connected, "ERR Undo is not possible now!\n");
         return;
      }

      current_game->undoLastMove();
      if(arduino_mode) Send_string(port_connected, "Last move was undone\n");
   }
}

//---------------------------------------------------------------------------------------
// Al Mover
//---------------------------------------------------------------------------------------
void AI_move(char fromC, char fromR, char toC, char toR, bool Is_promotion, char To_what)
{
   if (AI_mode)
   {
      cout << "Please wait(don't input please)...\n";
      string tmp; // temp to save the AI's reply
      string ans;
      ans.clear();
      string temp_command;
      temp_command = command;

      fstream my_file;
      my_file.open("temp_in.txt", std::ofstream::out | std::ofstream::trunc);
      if (my_file.is_open())
      { // checking whether the file is open
         my_file << temp_command << endl;
         my_file << 'd' << endl;
         my_file << "go movetime 4000\n";
         my_file << "go movetime 4000\n";
         my_file.close();
      }

      std::system("test.bat");
      cout << "AI thinking...\n";
      Sleep(5000);

      my_file.open("temp_out.txt"); // open a file to perform read operation using file object
      if (my_file.is_open())
      { // checking whether the file is open
         while (getline(my_file, tmp))
         { // read data from file object and put it into string.
            auto found = tmp.find("bestmove");
            if (found != string::npos)
            {
               if (Is_promotion)
               {
                  ans = tmp.substr(9, 5);
                  cout << ans << endl;
                  if(arduino_mode){
                     Send_string(port_connected, "Promote move!\n");
                     Send_string(port_connected, ans);
                  }
                  break;
               }
               else
               {
                  ans = tmp.substr(9, 4);
                  cout << ans << endl;
                  if(arduino_mode){
                  Send_string(port_connected, "Move\n");
                  Send_string(port_connected, ans);
                  }
                  break;
               }
            }
         }
         my_file.close(); // close the file object.
      }

      command += (ans);
      command.append(" ");

      std::string to_record;
      Chess::Position present;
      present.iColumn = toupper(ans[0]);
      present.iRow = toupper(ans[1]);

      // Put in the string to be logged
      to_record += present.iColumn;
      to_record += present.iRow;
      to_record += "-";

      // Convert column from ['A'-'H'] to [0x00-0x07]
      present.iColumn = present.iColumn - 'A';

      // Convert row from ['1'-'8'] to [0x00-0x07]
      present.iRow = present.iRow - '1';

      Chess::Position future;
      future.iColumn = toupper(ans[2]);
      future.iRow = toupper(ans[3]);
      // Put in the string to be logged
      to_record += future.iColumn;
      to_record += future.iRow;

      // Convert columns from ['A'-'H'] to [0x00-0x07]
      future.iColumn = future.iColumn - 'A';

      // Convert row from ['1'-'8'] to [0x00-0x07]
      future.iRow = future.iRow - '1';

      Chess::EnPassant S_enPassant = {0};
      Chess::Castling S_castling = {0};
      Chess::Promotion S_promotion = {0};

      isMoveValid(present, future, &S_enPassant, &S_castling, &S_promotion);

      if (S_promotion.bApplied == true)
      {
         std::string piece;
         piece.clear();
         piece += ans[4];
         char chPromoted = toupper(piece[0]);
         S_promotion.chBefore = current_game->getPieceAtPosition(present.iRow, present.iColumn);

         if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
         {
            S_promotion.chAfter = toupper(chPromoted);
         }
         else
         {
            S_promotion.chAfter = tolower(chPromoted);
         }
         to_record += '=';
         to_record += toupper(chPromoted); // always log with a capital letter
      }

      current_game->logMove(to_record);
      makeTheMove(present, future, &S_enPassant, &S_castling, &S_promotion);

      if (true == current_game->playerKingInCheck())
      {
         if (true == current_game->isCheckMate())
         {
            if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
            {
               appendToNextMessage("Checkmate! Black wins the game!\n");
               if(arduino_mode) Send_string(port_connected, "END Black win.\n");
            }
            else
            {
               appendToNextMessage("Checkmate! White wins the game!\n");
               if(arduino_mode) Send_string(port_connected, "END White win.\n");
            }
         }
         else
         {
            // Add to the string with '+=' because it's possible that
            // there is already one message (e.g., piece captured)
            if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
            {
               appendToNextMessage("White king is in check!\n");
               if(arduino_mode) Send_string(port_connected, "CHECK White king is in check!\n");
            }
            else
            {
               appendToNextMessage("Black king is in check!\n");
               if(arduino_mode) Send_string(port_connected, "CHECK Black king is in check!\n");
            }
         }
      }
      // clearScreen();
      printLogo();
      printSituation(*current_game);
      printBoard(*current_game);
   }
}

void movePiece(void)
{
   std::string to_record;
   // Get user input for the piece they want to move
   cout << "Choose piece to be moved. (example: A1 or b2): ";
   if(arduino_mode) Send_string(port_connected, "Player make move\n");

   if (arduino_mode)
   {
   }
   std::string move_from;
   getline(cin, move_from);
   // move_from = Read_line(port_connected);
   if (move_from.length() > 2)
   {
      createNextMessage("You should type only two characters (column and row)\n");
      if(arduino_mode) Send_string(port_connected, "ERR message too long");
      return;
   }

   Chess::Position present;
   present.iColumn = move_from[0];
   present.iRow = move_from[1];

   // ---------------------------------------------------
   // Did the user pick a valid piece?
   // Must check if:
   // - It's inside the board (A1-H8)
   // - There is a piece in the square
   // - The piece is consistent with the player's turn
   // ---------------------------------------------------
   present.iColumn = toupper(present.iColumn);
   if (present.iColumn < 'A' || present.iColumn > 'H')
   {
      createNextMessage("Invalid column.\n");
      if(arduino_mode) Send_string(port_connected, "ERR Invalid column.\n");
      return;
   }

   if (present.iRow < '0' || present.iRow > '8')
   {
      createNextMessage("Invalid row.\n");
      if(arduino_mode) Send_string(port_connected, "ERR Invalid column.\n");
      return;
   }

   // Put in the string to be logged
   to_record += present.iColumn;
   to_record += present.iRow;
   to_record += "-";

   // Convert column from ['A'-'H'] to [0x00-0x07]
   present.iColumn = present.iColumn - 'A';

   // Convert row from ['1'-'8'] to [0x00-0x07]
   present.iRow = present.iRow - '1';

   char chPiece = current_game->getPieceAtPosition(present.iRow, present.iColumn);
   cout <<"Piece is " << char(chPiece) << "\n";
   if(arduino_mode){
      string tempA = "Piece is ";
      tempA += char(chPiece);
      tempA += '\n';
      Send_string(port_connected, tempA);
   }
   if (0x20 == chPiece)
   {
      createNextMessage("You picked an EMPTY square.\n");
      if(arduino_mode) Send_string(port_connected, "ERR empty square\n");
      return;
   }

   if (Chess::WHITE_PIECE == current_game->getCurrentTurn())
   {
      if (false == Chess::isWhitePiece(chPiece))
      {
         createNextMessage("It is WHITE's turn and you picked a BLACK piece\n");
         if(arduino_mode) Send_string(port_connected, "ERR enemy piece\n");
         return;
      }
   }
   else
   {
      if (false == Chess::isBlackPiece(chPiece))
      {
         createNextMessage("It is BLACK's turn and you picked a WHITE piece\n");
         if(arduino_mode) Send_string(port_connected, "ERR enemy piece\n");
         return;
      }
   }

   // ---------------------------------------------------
   // Get user input for the square to move to
   // ---------------------------------------------------
   cout <<"Move to: ";
   std::string move_to;
   getline(cin, move_to);

   if (move_to.length() > 2)
   {
      cout << "error\n";
      createNextMessage("You should type only two characters (column and row)\n");
      if(arduino_mode) Send_string(port_connected, "ERR message is wrong\n");
      return;
   }

   // ---------------------------------------------------
   // Did the user pick a valid house to move?
   // Must check if:
   // - It's inside the board (A1-H8)
   // - The move is valid
   // ---------------------------------------------------
   Chess::Position future;
   future.iColumn = move_to[0];
   future.iRow = move_to[1];

   future.iColumn = toupper(future.iColumn);

   if (future.iColumn < 'A' || future.iColumn > 'H')
   {
      createNextMessage("Invalid column.\n");
      if(arduino_mode) Send_string(port_connected, "ERR Invalid column.\n");
      return;
   }

   if (future.iRow < '0' || future.iRow > '8')
   {
      createNextMessage("Invalid row.\n");
      if(arduino_mode) Send_string(port_connected, "ERR Invalid row.\n");
      return;
   }

   // Put in the string to be logged
   to_record += future.iColumn;
   to_record += future.iRow;

   // Convert columns from ['A'-'H'] to [0x00-0x07]
   future.iColumn = future.iColumn - 'A';

   // Convert row from ['1'-'8'] to [0x00-0x07]
   future.iRow = future.iRow - '1';

   // Check if it is not the exact same square
   if (future.iRow == present.iRow && future.iColumn == present.iColumn)
   {
      createNextMessage("[Invalid] You picked the same square!\n");
      if(arduino_mode) Send_string(port_connected, "ERR picked the same square.\n");
      return;
   }

   // Is that move allowed?
   Chess::EnPassant S_enPassant = {0};
   Chess::Castling S_castling = {0};
   Chess::Promotion S_promotion = {0};

   if (false == isMoveValid(present, future, &S_enPassant, &S_castling, &S_promotion))
   {
      createNextMessage("[Invalid] Piece can not move to that square!\n");
      if(arduino_mode) Send_string(port_connected, "ERR can not move to there\n");
      return;
   }

   // ---------------------------------------------------
   // Promotion: user most choose a piece to
   // replace the pawn
   // ---------------------------------------------------
   std::string piece;
   if (S_promotion.bApplied == true)
   {
      cout << "Promote to (Q, R, N, B): ";
      getline(cin, piece);
      if (piece.length() > 1)
      {
         createNextMessage("You should type only one character (Q, R, N or B)\n");
         if(arduino_mode) Send_string(port_connected, "ERR only one char for promotion\n");
         return;
      }

      char chPromoted = toupper(piece[0]);

      if (chPromoted != 'Q' && chPromoted != 'R' && chPromoted != 'N' && chPromoted != 'B')
      {
         createNextMessage("Invalid character.\n");
         if(arduino_mode) Send_string(port_connected, "ERR Invalid character\n");
         return;
      }

      S_promotion.chBefore = current_game->getPieceAtPosition(present.iRow, present.iColumn);

      if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
      {
         S_promotion.chAfter = toupper(chPromoted);
      }
      else
      {
         S_promotion.chAfter = tolower(chPromoted);
      }

      to_record += '=';
      to_record += toupper(chPromoted); // always log with a capital letter
   }
   // ---------------------------------------------------
   // Log the move: do it prior to making the move
   // because we need the getCurrentTurn()
   // ---------------------------------------------------
   current_game->logMove(to_record);

   // ---------------------------------------------------
   // Make the move
   // ---------------------------------------------------
   makeTheMove(present, future, &S_enPassant, &S_castling, &S_promotion);

   // ---------------------------------------------------
   // Telling the Arduino what the player moved
   // ---------------------------------------------------
   if(arduino_mode){
      string tempA;
      if(S_promotion.bApplied == true){
         tempA = move_from; tempA += " "; tempA += move_to; tempA += piece; tempA += '\n';
      }
      else{
         tempA = move_from; tempA += " "; tempA += move_to; tempA += '\n';
      }
   Send_string(port_connected, tempA);
   }
   // ---------------------------------------------------
   // Updating the command that should be given to the AI
   // ---------------------------------------------------
   command += (tolower(move_from[0]));
   command += (tolower(move_from[1]));
   command += (tolower(move_to[0]));
   command += (tolower(move_to[1]));
   if (S_promotion.bApplied == true)
   {
      command += (tolower(piece[0]));
   }
   command.append(" ");
   // ---------------------------------------------------
   // Attempt to call StockFish
   // ---------------------------------------------------
   if (AI_mode)
   {
      if (S_promotion.bApplied == true)
      {
         AI_move(move_from[0], move_from[1], move_to[0], move_to[1], 1, piece[0]);
      }
      else
      {
         AI_move(move_from[0], move_from[1], move_to[0], move_to[1], 0, 'n');
      }
   }
   // ---------------------------------------------------------------
   // Check if this move we just did put the oponent's king in check
   // Keep in mind that player turn has already changed
   // ---------------------------------------------------------------
   if (true == current_game->playerKingInCheck())
   {
      if (true == current_game->isCheckMate())
      {
         if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
         {
            appendToNextMessage("Checkmate! Black wins the game!\n");
            if(arduino_mode) Send_string(port_connected, "END Black win.\n");
         }
         else
         {
            appendToNextMessage("Checkmate! White wins the game!\n");
            if(arduino_mode) Send_string(port_connected, "END White win.\n");
         }
      }
      else
      {
         // Add to the string with '+=' because it's possible that
         // there is already one message (e.g., piece captured)
         if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
         {
            appendToNextMessage("White king is in check!\n");
            if(arduino_mode) Send_string(port_connected, "CHECK White king is in check!\n");
         }
         else
         {
            appendToNextMessage("Black king is in check!\n");
            if(arduino_mode) Send_string(port_connected, "CHECK Black king is in check!\n");
         }
      }
   }
   return;
}

void saveGame(void)
{
   string file_name;
   cout << "Type file name to be saved (no extension): ";
   if(arduino_mode) Send_string(port_connected, "SAVE file name\n");

   getline(cin, file_name);
   file_name += ".dat";

   std::ofstream ofs(file_name);
   if (ofs.is_open())
   {
      // Write the date and time of save operation
      auto time_now = std::chrono::system_clock::now();
      std::time_t end_time = std::chrono::system_clock::to_time_t(time_now);
      ofs << "[Chess console] Saved at: " << std::ctime(&end_time);

      // Write the moves
      for (unsigned i = 0; i < current_game->rounds.size(); i++)
      {
         ofs << current_game->rounds[i].white_move.c_str() << " | " << current_game->rounds[i].black_move.c_str() << "\n";
      }

      ofs.close();
      createNextMessage("Game saved as " + file_name + "\n");
      if(arduino_mode){
         string tempA;
         tempA = "SAVE Game saved as "; tempA += file_name; tempA += '\n';
         Send_string(port_connected, tempA);
      }
   }
   else
   {
      cout << "Error creating file! Save failed\n";
      if(arduino_mode) Send_string(port_connected, "SAVE ERR failed! \n");
   }

   return;
}

void loadGame(void)
{
   string file_name;
   cout << "Type file name to be loaded (no extension): ";
   if(arduino_mode) Send_string(port_connected, "LOAD file name\n");

   getline(cin, file_name);
   file_name += ".dat";

   std::ifstream ifs(file_name);

   if (ifs)
   {
      // First, reset the pieces
      if (NULL != current_game)
      {
         delete current_game;
      }

      current_game = new Game();

      // Now, read the lines from the file and then make the moves
      std::string line;

      while (std::getline(ifs, line))
      {
         // Skip lines that starts with "[]"
         if (0 == line.compare(0, 1, "["))
         {
            continue;
         }

         // There might be one or two moves in the line
         string loaded_move[2];

         // Find the separator and subtract one
         std::size_t separator = line.find(" |");

         // For the first move, read from the beginning of the string until the separator
         loaded_move[0] = line.substr(0, separator);

         // For the second move, read from the separator until the end of the string (omit second parameter)
         loaded_move[1] = line.substr(line.find("|") + 2);

         for (int i = 0; i < 2 && loaded_move[i] != ""; i++)
         {
            // Parse the line
            Chess::Position from;
            Chess::Position to;

            char chPromoted = 0;

            current_game->parseMove(loaded_move[i], &from, &to, &chPromoted);

            // Check if line is valid
            if (from.iColumn < 0 || from.iColumn > 7 ||
                from.iRow < 0 || from.iRow > 7 ||
                to.iColumn < 0 || to.iColumn > 7 ||
                to.iRow < 0 || to.iRow > 7)
            {
               createNextMessage("[Invalid] Can't load this game because there are invalid lines!\n");
               if(arduino_mode) Send_string(port_connected, "LOAD ERR failed!(invalid lines) \n");

               // Clear everything and return
               current_game = new Game();
               return;
            }

            // Is that move allowed? (should be because we already validated before saving)
            Chess::EnPassant S_enPassant = {0};
            Chess::Castling S_castling = {0};
            Chess::Promotion S_promotion = {0};

            if (false == isMoveValid(from, to, &S_enPassant, &S_castling, &S_promotion))
            {
               createNextMessage("[Invalid] Can't load this game because there are invalid moves!\n");
               if(arduino_mode) Send_string(port_connected, "LOAD ERR failed!(invalid moves) \n");

               // Clear everything and return
               current_game = new Game();
               return;
            }

            // ---------------------------------------------------
            // A promotion occurred
            // ---------------------------------------------------
            if (S_promotion.bApplied == true)
            {
               if (chPromoted != 'Q' && chPromoted != 'R' && chPromoted != 'N' && chPromoted != 'B')
               {
                  createNextMessage("[Invalid] Can't load this game because there is an invalid promotion!\n");
                  if(arduino_mode) Send_string(port_connected, "LOAD ERR failed!(invalid promotion) \n");

                  // Clear everything and return
                  current_game = new Game();
                  return;
               }

               S_promotion.chBefore = current_game->getPieceAtPosition(from.iRow, from.iColumn);

               if (Chess::WHITE_PLAYER == current_game->getCurrentTurn())
               {
                  S_promotion.chAfter = toupper(chPromoted);
               }
               else
               {
                  S_promotion.chAfter = tolower(chPromoted);
               }
            }

            // Log the move
            current_game->logMove(loaded_move[i]);

            // Make the move
            makeTheMove(from, to, &S_enPassant, &S_castling, &S_promotion);
         }
      }

      // Extra line after the user input
      createNextMessage("Game loaded from " + file_name + "\n");
      if(arduino_mode){
         string tempA;
         tempA = "LOAD Game loaded from "; tempA += file_name; tempA += "\n";
         Send_string(port_connected, tempA);
      }
      return;
   }
   else
   {
      createNextMessage("Error loading " + file_name + ". Creating a new game instead\n");
      if(arduino_mode){
         string tempA;
         tempA = "LOAD Error loading "; tempA += file_name; tempA += ". Creating a new game instead\n";
         Send_string(port_connected, tempA);
      }
      current_game = new Game();
      return;
   }
}

int main()
{
   bool bRun = true;

   // Clear screen an print the logo
   clearScreen();
   printLogo();

   string input = "";

   while (bRun)
   {
      printMessage();
      printMenu();
      cout << "Type here: ";
      getline(cin, input);
      if (input.length() != 1)
      {
         cout << "Invalid option. Type one letter only\n\n";
         cout << "you typed: " << input << '\n';
         if(arduino_mode) Send_string(port_connected, "ERR invalid option\n");
         continue;
      }
      try
      {
         switch (input[0])
         {
         case 'N':
         case 'n':
         {
            newGame();
            clearScreen();
            printLogo();
            printSituation(*current_game);
            printBoard(*current_game);
         }
         break;

         case 'M':
         case 'm':
         {
            if (NULL != current_game)
            {
               if (current_game->isFinished())
               {
                  cout << "This game has already finished!\n";
                  if(arduino_mode) Send_string(port_connected, "ERR game has ended.\n");
               }
               else
               {
                  movePiece();
                  // clearScreen();
                  printLogo();
                  printSituation(*current_game);
                  printBoard(*current_game);
               }
            }
            else
            {
               cout << "No game running!\n";
               if(arduino_mode) Send_string(port_connected, "ERR no game running.\n");
            }
         }
         break;

         case 'Q':
         case 'q':
         {
            if(arduino_mode) Send_string(port_connected, "Quit\n");
            command = "position startpos move ";
            bRun = false;
         }
         break;

         case 'U':
         case 'u':
         {
            if (NULL != current_game)
            {
               undoMove();
               // clearScreen();
               printLogo();
               printSituation(*current_game);
               printBoard(*current_game);
            }
            else
            {
               cout << "No game running\n";
               if(arduino_mode) Send_string(port_connected, "ERR no game running(U).\n");
            }
         }
         break;

         case 'S':
         case 's':
         {
            if (NULL != current_game)
            {
               saveGame();
               clearScreen();
               printLogo();
               printSituation(*current_game);
               printBoard(*current_game);
            }
            else
            {
               cout << "No game running\n";
               if(arduino_mode) Send_string(port_connected, "ERR no game running(S)\n");
            }
         }
         break;

         case 'L':
         case 'l':
         {
            loadGame();
            clearScreen();
            printLogo();
            printSituation(*current_game);
            printBoard(*current_game);
         }
         break;

         case 'k':
         case 'K':
         {
            cout << "Type here: ";
            getline(cin, input);
            if (input == "By the name of KoalaWill, I'll cast the spell of Fatality. May the darkest emerge from their abode, and shall Thanatos grant you the end !")
            {
               cout << "\nthe game has been shut down by a powerful curse, yeet !!\n";
               getchar();
               bRun = false;
               break;
            }
         }
         break;

         case 'a':
         case 'A':
         {
            if (AI_mode)
            {
               AI_mode = 0;
               cout << "AI mode off\n";
               if(arduino_mode) Send_string(port_connected, "AI off\n");
            }
            else
            {
               AI_mode = 1;
               cout << "AI mode on\n";
               if(arduino_mode) Send_string(port_connected, "AI on\n");
            }
         }
         break;

         case 'd':
         case 'D':
         {
            if (arduino_mode)
            {
               arduino_mode = 0;
               cout << "Arduino mode off\n";
            }
            else
            {
               cout << "testing\n";
               Scan_COMS();
               Send_string(port_connected, "testing...\n");
               arduino_mode = 1;
               cout << "Arduino mode on\n";
            }
         }
         break;

         default:
         {
            cout << "Option does not exist\n\n";
            if(arduino_mode) Send_string(port_connected, "ERR no such option.\n");
         }
         break;
         }
      }
      catch (const char *s)
      {
         s;
      }
   }

   return 0;
}
