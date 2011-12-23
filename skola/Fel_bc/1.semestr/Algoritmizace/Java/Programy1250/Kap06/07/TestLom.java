// Soubor Kap06\07\TestLom.java
// Balen� ve spojitosti s parametizovan�mi kontejnery
// Jen JDK 5

import java.util.ArrayList;

// T��da p�edstavuj�c� lomenou ��ru
class LomenaCara
{
  private ArrayList<Integer> X, Y;
  public LomenaCara(int[] XX, int[] YY)
  {
    X = new ArrayList<Integer>(); 
    Y = new ArrayList<Integer>();
    int i = 0;
    while(i < XX.length)
    {
      X.add(XX[i]);
      Y.add(YY[i++]);
    }
  } 
  public void nakresli()
  {
    // Obl�ben� tik: kreslen� nahrad�me v�pisem
    System.out.print("Lomena cara [");
    int i = 0;
    while(i < X.size())
    {
      System.out.printf("(%d, %d)", X.get(i), Y.get(i));
      i++;
    }
    System.out.println("]");
  }
  // metody pro zm�nu X, resp. Y o D
  // Vyu��vaj� balen� a vybalov�n�
  public void posunX(int d)
  {
    int i = 0;
    while(i < X.size())
    {
      X.add(i, X.remove(i)+d);
      i++;
    }
  }
  public void posunY(int d)
  {
    int i = 0;
    while(i < Y.size())
    {
      Y.add(i, Y.remove(i)+d);
      i++;
    }
  }

}

public class TestLom
{
  public static void main(String[] s)
  {
    int[] A = {1, 3, 6};
    int[] B = {2, 4, 7};
    LomenaCara lc = new LomenaCara(A, B);
    lc.nakresli();
    lc.posunX(5);
    lc.nakresli();   
  }
}