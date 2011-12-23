package alg6;

import java.util.*;

public class Eratosthen {
  public static void main(String[] args) {
    Scanner sc = new Scanner(System.in);
    System.out.println("zadejte max");
    int max = sc.nextInt();
    boolean[] mnozina = sito(max);
    System.out.println("prvo��sla od 2 do "+max);
    vypis(mnozina);
  }

  static boolean[] sito(int max) {
    boolean[] mnozina = new boolean[max+1];
    for (int i=2; i<=max; i++) mnozina[i] = true;
    int p = 2;
    int pmax = (int)Math.sqrt(max);
    do {
      // vypu�t�n� v�ech n�sobk� ��sla p
      for (int i=p+p; i<=max; i+=p) mnozina[i] = false;
      // hled�n� nejbli���ho ��sla k p
      do {
        p++;
      } while (!mnozina[p]);
    } while (p<=pmax);
    return mnozina;
  }

  static void vypis(boolean[] mnozina) {
    for (int i=2; i<mnozina.length; i++)
      if (mnozina[i]) System.out.println(i);
  }

}