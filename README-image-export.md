# Image Export Feature

This branch adds a library-level function to export a PNG image of your Top 100 list as a 10×10 grid with a heading.

API:
- bool exportTop100Image(const std::vector<Movie>& movies, const std::string& outPath, const std::string& heading = "My Top 100 Movies");

Build requirements:
- Cairo (cairo) for image composition. If not found at configure time, the function will return false at runtime.
- cpr is used to fetch poster images via HTTP(S). SSL must be enabled in cpr (OpenSSL or similar present).

Notes:
- Posters are fetched from Movie::posterUrl. Missing or non-PNG posters will render a placeholder.
- Titles are elided to fit in a single line per cell.
- Up to 100 movies are rendered. Fewer movies are supported.
