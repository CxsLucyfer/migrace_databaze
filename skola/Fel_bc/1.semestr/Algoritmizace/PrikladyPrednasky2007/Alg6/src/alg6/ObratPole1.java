package alg6;
import java.util.*;

public class ObratPole1 {
  public static void main(String[] args) {
      Scanner sc = new Scanner(System.in);
      System.out.println("zadejte po�et ��sel");
    int[] pole = new int[sc.nextInt()];
    System.out.println("zadejte "+pole.length+" ��sel");
    for (int i=0; i<pole.length; i++)
      pole[i] = sc.nextInt();
    System.out.println("v�pis ��sel v obr�cen�m po�ad�");
    for (int i=pole.length-1; i>=0; i--)
      System.out.println(pole[i]);
  }
}
