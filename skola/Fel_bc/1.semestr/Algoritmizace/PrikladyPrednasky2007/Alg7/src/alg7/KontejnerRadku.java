package alg7;

import java.util.ArrayList;
import java.util.*;


public class KontejnerRadku {
  public static void main(String[] args) {
    Scanner sc = new Scanner(System.in);
    ArrayList radky = new ArrayList();
    System.out.println("zadejte ��dky zakon�en� pr�zdn�m ��dkem");
    String radek = sc.nextLine();
    while (!radek.equals("")) {
      radky.add(radek);
      radek = sc.nextLine();
    }
    System.out.println("v�pis ��dk� v opa�n�m po�ad�");
    for (int i=radky.size()-1; i>=0; i--)
      System.out.println(radky.get(i));
    String prvni = (String)radky.get(0);
  }
}
