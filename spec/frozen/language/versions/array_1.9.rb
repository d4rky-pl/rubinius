describe "The unpacking splat operator (*)" do
  it "when applied to a non-Array value attempts to coerce it to Array if the object respond_to?(:to_a)" do
    obj = mock("pseudo-array")
    obj.should_receive(:to_a).and_return([2, 3, 4])
    [1, *obj].should == [1, 2, 3, 4]
  end

  it "when applied to a non-Array value uses it unchanged if it does not respond_to?(:to_a)" do
    obj = Object.new
    obj.should_not respond_to(:to_a)
    [1, *obj].should == [1, obj]
  end

  it "can be used before other non-splat elements" do
    a = [1, 2]
    [0, *a, 3].should == [0, 1, 2, 3]
  end

  it "can be used multiple times in the same containing array" do
    a = [1, 2]
    b = [1, 0]
    [*a, 3, *a, *b].should == [1, 2, 3, 1, 2, 1, 0]
  end
end
