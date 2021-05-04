package edu.neu.mad_sea.jdobrowolski.tictactoe;

public class TicTacToeModel {

    /**
     * Counts:
     *    - playerOneScoreCount: counts number of games one by player one
     *    - playerTwoScoreCount: counts number of games won by player two
     *    - roundCount: number of rounds played during a game
     */
    private int playerOneScoreCount, playerTwoScoreCount, roundCount;

    /**
     * activePlayer: toggles between player one and two
     *    - true: player one's turn
     *    - false: player two's turn
     */
    private boolean activePlayer;

    /**
     * Game State: the state will be controlled by an array of integers
     *    - 0: player one has selected this game piece (or button)
     *    - 1: player two has selected this game piece (or button)
     *    - 2: neither player has chosen this game piece and is available for use
     */
    private int[] gameState = {2,2,2,2,2,2,2,2,2};

    /**
     * Possible winning positions, think of your game board as the following:
     *    0   1   2
     *    3   4   5
     *    6   7   8
     */
    private int[][] winningPositions = {
            {0,1,2}, {3,4,5}, {6,7,8}, // rows
            {0,3,6}, {1,4,7}, {2,5,8}, // columns
            {0,4,8}, {2,4,6}           // diagonal
    };

    public TicTacToeModel() {
        roundCount = 0;
        playerOneScoreCount = 0;
        playerTwoScoreCount = 0;
        activePlayer = true;
    }

    // Checks for a winner.
    public boolean checkWinner() {
        boolean winnerResult = false;

        for(int[] winningPosition: winningPositions) {
            if(gameState[winningPosition[0]] == gameState[winningPosition[1]] &&
                    gameState[winningPosition[1]] == gameState[winningPosition[2]] &&
                    gameState[winningPosition[0]] != 2) {
                winnerResult = true;
            }
        }

        return winnerResult;
    }

    public boolean getActivePlayer() {
        return activePlayer;
    }

    public void changeActivePlayer() {
        activePlayer = !activePlayer;
    }

    public int getRoundCount() {
        return roundCount;
    }

    public void incrementRound() {
        roundCount++;
    }


    /**
     * Starts a new games with a new board and resets the score.
     */
    public void reset() {
        playerOneScoreCount = 0;
        playerTwoScoreCount = 0;
        nextGame();

    }

    /**
     * Starts a new games with a new board but does not modify the cumulative score
     */
    public void nextGame() {
        roundCount = 0;
        activePlayer = true;

        for (int i = 0; i < gameState.length; i++) {
            gameState[i] = 2;
        }
    }

    public int getPlayer1Score() {
        return playerOneScoreCount;
    }

    public int getPlayer2Score() {
        return playerTwoScoreCount;
    }

    public void pointForPlayer1() {
        playerOneScoreCount++;
    }

    public void pointForPlayer2() {
        playerTwoScoreCount++;
    }

    public void setP1Score(int score) {
        playerOneScoreCount = score;
    }

    public void setP2Score(int score) {
        playerTwoScoreCount = score;
    }

    public void nextTurn() {
        roundCount++;
        System.out.println("Round updated to " + roundCount);
    }

    public void setTurn(int turn) {
        roundCount = turn;
    }

    public int getTurn() {
        return roundCount;
    }

    public void setGameTile(int gameStatePointer) {
        gameState[gameStatePointer] = getActivePlayer() ? 0 : 1;
    }

    public void setGameState(int[] gameState) {
        this.gameState = gameState;
    }

}
