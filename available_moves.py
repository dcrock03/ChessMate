## Todo make a board object, add input since it will eventually be needed
## also need to add a way to stop pieces from moving after it hits a piece,
## add a way to determine checks and how to move, defended pieces that the king can't take,
## first pawn move, en passant, and castling

def print_Board(Board, rows, cols):
    for x in range(rows):
        for y in range(cols):
            print(Board[x][y], end=" ")
        print()
def loc_avlb(row, col):
   if( 0<=row<=7 and 0<=col<=7):
      return 1
   else:
      return 0
class Location:
  def __init__(self, row, col):
    self.row = row
    self.col = col
def clear_Board(Board, rows, cols):
   for x in range(rows):
      for y in range(cols):
         Board[x][y] = 0

(rows, cols) = (8, 8)
Board = [[0 for x in range(rows)] for y in range(cols)] 


def move_King(Board, loc):
    Board[loc.row+1][loc.col+1] = loc_avlb(loc.row+1,loc.col+1)
    Board[loc.row+1][loc.col] = loc_avlb(loc.row+1,loc.col)
    Board[loc.row+1][loc.col-1] = loc_avlb(loc.row+1,loc.col-1)
    Board[loc.row][loc.col+1] = loc_avlb(loc.row,loc.col+1)
    Board[loc.row][loc.col-1] = loc_avlb(loc.row,loc.col-1)
    Board[loc.row-1][loc.col+1] = loc_avlb(loc.row-1,loc.col+1)
    Board[loc.row-1][loc.col] = loc_avlb(loc.row-1,loc.col)
    Board[loc.row-1][loc.col-1] = loc_avlb(loc.row-1,loc.col-1)

Board[0][0] = "X"
loc = Location(0,0)
move_King(Board, loc)
print("King test: ")
print_Board(Board, rows, cols)
clear_Board(Board, rows, cols)

def move_Rook(Board, loc): ##TODO DJC add a method so that squares past a location are not occupied
   row = loc.row
   col = loc.col
   while(row<7):
      row = row + 1
      Board[row][loc.col] = loc_avlb(row,loc.col)
    
   row = loc.row
   while(row>=1):
      row = row - 1
      Board[row][loc.col] = loc_avlb(row,loc.col)

   while(col<7):
      col = col + 1
      Board[loc.row][col] = loc_avlb(loc.row,col)
    
   col = loc.col
   while(col>=1):
      col = col - 1
      Board[loc.row][col] = loc_avlb(loc.row,col)
    
Board[4][4] = "X"
loc = Location(4,4)
move_Rook(Board, loc)
print("Rook test: ")
print_Board(Board, rows, cols)
clear_Board(Board, rows, cols)

def move_Knight(Board, loc):
   Board[loc.row+2][loc.col+1] = loc_avlb(loc.row+2,loc.col+1)
   Board[loc.row+2][loc.col-1] = loc_avlb(loc.row+2,loc.col-1)
   Board[loc.row-2][loc.col+1] = loc_avlb(loc.row-2,loc.col+1)
   Board[loc.row-2][loc.col-1] = loc_avlb(loc.row-2,loc.col-1)
   Board[loc.row+1][loc.col+2] = loc_avlb(loc.row+1,loc.col+2)
   Board[loc.row-1][loc.col+2] = loc_avlb(loc.row-1,loc.col+2)
   Board[loc.row+1][loc.col-2] = loc_avlb(loc.row+1,loc.col-2)
   Board[loc.row-1][loc.col-2] = loc_avlb(loc.row-1,loc.col-2)

Board[3][1] = "X"
loc = Location(3,1)
move_Knight(Board, loc)
print("Knight test: ")
print_Board(Board, rows, cols)
clear_Board(Board, rows, cols)

def move_Bishop(Board, loc):
   row = loc.row
   col = loc.col
   while(col < 7 and row < 7):
      row = row + 1
      col = col + 1
      Board[row][col] = loc_avlb(row, col)
   row = loc.row
   col = loc.col
   while(col > 0 and row < 7):
      row = row + 1
      col = col - 1
      Board[row][col] = loc_avlb(row, col)
   row = loc.row
   col = loc.col
   while(col < 7 and row > 0):
      row = row - 1
      col = col + 1
      Board[row][col] = loc_avlb(row, col)
   row = loc.row
   col = loc.col
   while(col > 0 and row > 0):
      row = row - 1
      col = col - 1
      Board[row][col] = loc_avlb(row, col)

Board[5][5] = "X"
loc = Location(5,5)
move_Bishop(Board, loc)
print("Bishop test: ")
print_Board(Board, rows, cols)
clear_Board(Board, rows, cols)

def move_Queen(Board, loc):
   move_Bishop(Board, loc)
   move_Rook(Board, loc)

Board[3][5] = "X"
loc = Location(3,5)
move_Queen(Board, loc)
print("Queen test: ")
print_Board(Board, rows, cols)
clear_Board(Board, rows, cols)