package alg12;

// Zasobn�k znak� implementovan� pomoc� pole

public class ZasobnikZnaku {
  static final int MAXINDEX = 99;
  private char[] pole;
  private int vrchol;

  public ZasobnikZnaku() {
    pole = new char[MAXINDEX+1];
    vrchol = -1;
  }

  public void vloz(char z) {
    if (vrchol==MAXINDEX)
      throw new RuntimeException("vlo�en� do pln�ho z�sobn�ku");
    pole[++vrchol] = z;
  }

  public char odeber() {
    if (jePrazdny())
      throw new RuntimeException("odebr�n� z pr�zdn�ho z�sobn�ku");
    return pole[vrchol--];
  }

  public boolean jePrazdny() {
    return vrchol<0;
  }


}
