package alg4;

import java.util.*;

public class StatickePromenne {
  
  static int x, y;

  public static void main(String[] args) {
    Scanner sc = new Scanner(System.in);
    System.out.println("zadejte dv� cel� ��sla");
    x = sc.nextInt();
    y = sc.nextInt();
    vypisSoucet();
  }

  static void vypisSoucet() {
     System.out.println("sou�et ��sel je "+(x+y));
  }

}
