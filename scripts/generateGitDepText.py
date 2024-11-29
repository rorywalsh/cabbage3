import hashlib
import argparse

def generate_github_dependency(repo_url, git_tag):
    # Compute MD5 checksum of the URL
    url_md5 = hashlib.md5(repo_url.encode()).hexdigest()
    
    # Create the output string
    output = f"""fetch_github_dependency(
    iplug2
    GIT_REPOSITORY {repo_url}
    GIT_TAG {git_tag}
    URL_MD5 {url_md5}
    # USE_GIT
)"""
    return output

if __name__ == "__main__":
    # Set up argument parsing
    parser = argparse.ArgumentParser(description="Generate GitHub dependency script.")
    parser.add_argument("repo_url", help="The Git repository URL.")
    parser.add_argument("git_tag", help="The Git tag or SHA.")
    
    # Parse the arguments
    args = parser.parse_args()
    
    # Generate and print the dependency script
    result = generate_github_dependency(args.repo_url, args.git_tag)
    print(result)
