package edu.neu.mad_sea.jdobrowolski.tictactoe;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class GameState {
    private String gameNumber;
    private boolean player;
    private int col;
    private int row;
    private String p1Name;
    private String p2Name;
    private int activePlayers;
    private List<String> board;
    private int turnNumber;
    private int p1Score;
    private int p2Score;



    public GameState() {
        player = true;
        col = -1;
        row = -1;
        p1Name = "";
        p2Name = "";
        activePlayers = 0;

        String[] temp = new String[9];
        for (int i = 0; i < temp.length; ++i) {
            temp[i] = "";
        }

        board = new ArrayList<String>(Arrays.asList(temp));
    }

    public boolean getPlayer() {
        return player;
    }

    public void setPlayer(boolean player) {
        this.player = player;
    }

    public int getCol() {
        return col;
    }

    public void setCol(int col) {
        this.col = col;
    }

    public int getRow() {
        return row;
    }

    public void setRow(int row) {
        this.row = row;
    }

    public String getP1Name() {
        return p1Name;
    }

    public void setP1Name(String p1Name) {
        this.p1Name = p1Name;
    }

    public String getP2Name() {
        return p2Name;
    }

    public void setP2Name(String p2Name) {
        this.p2Name = p2Name;
    }

    public int getActivePlayers() {
        return activePlayers;
    }

    public void setActivePlayers(int activePlayers) {
        this.activePlayers = activePlayers;
    }

    public String getGameNumber() {
        return gameNumber;
    }

    public void setGameNumber(String gameNumber) {
        this.gameNumber = gameNumber;
    }

    public List<String> getBoard() {
        return this.board;
    }

    public void setBoard(List<String> board) {
        this.board = board;
    }

    public int getTurnNumber() {
        return turnNumber;
    }

    public void setTurnNumber(int turnNumber) {
        this.turnNumber = turnNumber;
    }

    public int getP1Score() {
        return p1Score;
    }

    public void setP1Score(int p1Score) {
        this.p1Score = p1Score;
    }

    public int getP2Score() {
        return p2Score;
    }

    public void setP2Score(int p2Score) {
        this.p2Score = p2Score;
    }
}
