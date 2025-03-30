import os

def create_matrix(matrix_id):
    """Create a 10x10 matrix with small floating-point values based on matrix_id"""
    return [[((i + j + matrix_id) % 9 + 1) * 0.1 for j in range(12)] for i in range(10)]

def matrix_multiply(a, b):
    """Naive matrix multiplication"""
    return [[sum(a[i][k] * b[k][j] for k in range(10)) for j in range(12)] for i in range(10)]

def matrix_power(matrix, power):
    """Raise matrix to the specified power"""
    result = matrix
    for _ in range(power - 1):
        result = matrix_multiply(result, matrix)
    return result

def print_matrix(matrix, matrix_id):
    """Print a matrix with its ID"""
    print(f"\nMatrix {matrix_id} to the 20th power (first row):")
    print(matrix[0])  # Show just first row for brevity

def main():
    # Get bounds from environment
    try:
        lower = int(os.getenv('PROCESS_BOUND_LOWER', 0))
        upper = int(os.getenv('PROCESS_BOUND_UPPER', 1000))
    except ValueError:
        lower = 0
        upper = 1000
        return


    # Create 9 hardcoded 12x12 matrices
    matrices = [create_matrix(i) for i in range(1000)]

    # Raise each matrix to the 10th power within bounds
    for i in range(lower, min(upper, len(matrices))):
        powered_matrix = matrix_power(matrices[i], 20)
        print_matrix(powered_matrix, i)

if __name__ == "__main__":
    main()
