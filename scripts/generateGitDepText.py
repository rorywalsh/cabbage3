import hashlib
import argparse
import requests
import os

def download_file(url, output_path):
    """Download a file from a URL to the specified path."""
    response = requests.get(url, stream=True)
    response.raise_for_status()  # Raise an error for failed requests
    with open(output_path, "wb") as f:
        for chunk in response.iter_content(chunk_size=8192):
            f.write(chunk)

def compute_md5(file_path):
    """Compute the MD5 hash of a file."""
    md5 = hashlib.md5()
    with open(file_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            md5.update(chunk)
    return md5.hexdigest()

def construct_download_url(repo_url, git_tag):
    """Construct the download URL by modifying the repo URL and replacing hdacode with the git_tag."""
    if not repo_url.endswith(".git"):
        repo_url += ".git"
    download_url = repo_url[:-4] + f"/archive/{git_tag}.tar.gz"  # Remove .git and append /{git_tag}.tar.gz
    return download_url

def generate_github_dependency(repo_url, git_tag):
    # Construct the download URL
    download_url = construct_download_url(repo_url, git_tag)
    temp_file = "temp_archive.tar.gz"

    try:
        # Download the archive
        print(f"Downloading archive from: {download_url}")
        download_file(download_url, temp_file)
        
        # Compute MD5 checksum of the downloaded file
        url_md5 = compute_md5(temp_file)
        
        # Create the output string
        output = f"""fetch_github_dependency(
    iplug2
    GIT_REPOSITORY {repo_url}
    GIT_TAG {git_tag}
    URL_MD5 {url_md5}
    # USE_GIT
)"""
        return output
    finally:
        # Clean up temporary file
        if os.path.exists(temp_file):
            os.remove(temp_file)

if __name__ == "__main__":
    # Set up argument parsing
    parser = argparse.ArgumentParser(description="Generate GitHub dependency script.")
    parser.add_argument("repo_url", help="The Git repository URL (must include or will append .git).")
    parser.add_argument("git_tag", help="The Git tag or SHA.")
    
    # Parse the arguments
    args = parser.parse_args()
    
    # Generate and print the dependency script
    result = generate_github_dependency(args.repo_url, args.git_tag)
    print(result)
