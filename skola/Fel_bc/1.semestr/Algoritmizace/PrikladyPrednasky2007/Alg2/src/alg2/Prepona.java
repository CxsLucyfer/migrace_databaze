package alg2;

import java.util.*;



public class Prepona {
  public static void main(String[] args) {
      Scanner sc = new Scanner(System.in);
   System.out.println("zadejte d�lku odv�sen pravo�hl�ho troj�heln�ka");
    double x = sc.nextDouble();
    double y = sc.nextDouble();
    double z = Math.sqrt(x*x+y*y);
   System.out.println("d�lka p�epony je "+z);
  }
}