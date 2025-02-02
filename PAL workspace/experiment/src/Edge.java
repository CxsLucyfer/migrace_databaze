



class Edge implements Comparable<Edge>{
	public int input ;
	public int output ;
	public int weight ;
	public int algorithmWeight;
	
	public Edge fParent;
	public Edge [] fChildren;
	public boolean deleted;
	
	private static Edge dummyEdge;
	
	//dummyEdge
	public static Edge getDummyEdge(){
		if(dummyEdge == null){
			dummyEdge = new Edge(Integer.MIN_VALUE, Integer.MIN_VALUE, Integer.MIN_VALUE, Integer.MIN_VALUE);
		}
		return dummyEdge;
	}
	
	public Edge(int input, int output, int weight, int maxChildren){
		this.input = input;
		this.output = output;
		this.weight = weight;
		this.algorithmWeight = weight;
		fChildren = new Edge[maxChildren];
	}

	@Override
	public int compareTo(Edge o) {
		if(this.weight < o.weight){
			return -1;
		}
		if(this.weight > o.weight){
			return 1;
		}
		return 0;
	}
	
	public String toString(){
		return "in:" + this.input + " out:" + this.output + " w:" + weight + " aw:"+algorithmWeight;
	}
	
	
}