/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package NavrhovyVzorState;

import Core.Matrix;
import GUI.ResultWin;

/**
 *
 * @author já
 */
public interface AbstractChoice {
   abstract public void setChosenCommand(Command command);
   abstract public void count(Matrix firstMatrix,Matrix secondMatrix, float multiplicator, ResultWin window);
}
