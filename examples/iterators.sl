var it = [3, 4, 5].iterator()

while it.hasNext() do
  print(it.next())

it = (1..5).iterator()

while it.hasNext() do
  print(it.next())

it = (1..<5).iterator()

while it.hasNext() do
  print(it.next())
