package alg3;

import java.util.*;

public class Prvocislo {
  public static void main(String[] args) {
      Scanner sc = new Scanner(System.in);
      int n, d, sqrtn;
    boolean jePrvocislo = true;
      System.out.println("zadejte p�irozen� ��slo");
    n = sc.nextInt();
    if (n<1) {
      System.out.println(n + " nen� p�irozen� ��slo");
      System.exit(0);
    }
    if (n>2)
      if (n%2 == 0)
        jePrvocislo = false;
      else {
        sqrtn = (int)Math.sqrt(n); d = 3;
        while (jePrvocislo && (d<=sqrtn)) {
          if (n%d == 0) jePrvocislo = false;
          else d += 2;
        }
      }
    if (jePrvocislo)
      System.out.println(n+" je prvo��slo");
    else
      System.out.println(n+" nen� prvo��slo");
  }
}